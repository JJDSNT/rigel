#include "harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m68k.h"
#include "rigel/rigel.h"
#include "rigel/rigel_audio.h"
#include "rigel/rigel_bus.h"
#include "rigel/rigel_serial.h"
#include "rigel/rigel_time.h"

#include "hunk.h"

#include "expansions/fastram.h"
#include "expansions/lide.h"
#include "expansions/zorro.h"

/* Global pointer used by Musashi callbacks (Musashi has no userdata slot). */
static harness_t *g_harness;

enum {
    /*
     * PC unchanged across this many steps *and* no interrupt in that whole
     * stretch: treat the CPU as wedged. Musashi exposes no "stopped" query, so
     * a wedged CPU and a STOP waiting on the next VBLANK both show a frozen
     * PC — the interrupt traffic is what tells them apart.
     */
    HARNESS_HALT_STEPS = 4096
};

struct harness_t {
    RigelContext *rigel;
    rigel_config_t config;

    uint8_t *chip_ram;
    uint32_t chip_ram_size;
    uint8_t *slow_ram;
    uint32_t slow_ram_size;
    /*
     * ROM windows. A 1 MB image occupies both: the first half answers at
     * 0xE00000 and the second at 0xF80000. Smaller images use the standard
     * window only, mirrored by std_mask.
     */
    uint8_t *rom;
    uint32_t rom_size;
    uint32_t std_off, std_size, std_mask;
    uint32_t ext_off, ext_size, ext_mask;
    uint32_t ovl_off, ovl_mask;   /* what the overlay puts at address 0 */

    uint32_t *framebuffer;   /* HARNESS_MAX_WIDTH * HARNESS_MAX_HEIGHT RGBA8888 */

    uint8_t *adf[4];         /* floppy images, owned here */

    uint64_t cpu_cycles_total;

    /*
     * CPU/chipset clock integrator. Rigel counts CCK (~3.55 MHz), Musashi
     * counts CPU cycles (~7.09 MHz); cck_rem carries the odd CPU cycle across
     * timeslices so the /2 conversion never loses time.
     */
    uint32_t cck_rem;
    int      cycles_flushed;  /* m68k_cycles_run() at the last flush */
    uint32_t pending_events;  /* Rigel events seen since the last harness_step */

    uint32_t last_pc;
    unsigned pc_stall_steps;

    uint8_t last_ipl;         /* level currently asserted into Musashi */
    bool    exec_mode;        /* running a hunk image, so there is no ROM */
    uint32_t exec_fb, exec_fb_w, exec_fb_h, exec_fb_pitch;
    uint32_t exec_exit;       /* the park-here address the entry returns to */

    harness_serial_fn serial_fn;
    void             *serial_opaque;

    harness_mmio_fn   mmio_fn;
    void             *mmio_opaque;

    harness_audio_fn  audio_fn;
    void             *audio_opaque;
    uint32_t          audio_rate;
    /* Fixed-point CCK-per-output-sample accumulator, 16.16. Rigel's clock is
     * not a multiple of any audio rate, so an integer divisor would drift
     * audibly over a few seconds. */
    uint32_t          audio_step_q16;
    uint32_t          audio_accum_q16;

    lide_board_t     *lide;
    fastram_board_t  *fastram;
};

/* -------------------------------------------------------------------------
 * Chip RAM callbacks wired into rigel_config_t
 * ------------------------------------------------------------------------- */

static uint16_t chip_ram_read16(void *opaque, uint32_t addr)
{
    harness_t *h = (harness_t *)opaque;
    if (addr + 1 >= h->chip_ram_size) return 0;
    return (uint16_t)((h->chip_ram[addr] << 8) | h->chip_ram[addr + 1]);
}

static void chip_ram_write16(void *opaque, uint32_t addr, uint16_t value)
{
    harness_t *h = (harness_t *)opaque;
    if (addr + 1 >= h->chip_ram_size) return;
    h->chip_ram[addr]     = (uint8_t)(value >> 8);
    h->chip_ram[addr + 1] = (uint8_t)(value & 0xFF);
}

/* Rigel falls back to stderr when no sink is installed, and the compositor
 * emits one event per composed line. Swallow them unless the caller asked for
 * the trace. */
static void harness_log_event_sink(const rigel_log_event_t *event, void *opaque)
{
    (void)event;
    (void)opaque;
}

/* -------------------------------------------------------------------------
 * Clock integrator
 *
 * Every chipset-visible access flushes the CPU cycles run so far into Rigel
 * first, so a VPOS/VHPOS/INTREQ read reflects where the beam actually is
 * rather than where it was at the top of the timeslice.
 * ------------------------------------------------------------------------- */

/*
 * IPL is a level, not an edge. Republishing it only on RIGEL_EVENT_IRQ_CHANGED
 * loses every transition that a register write causes between steps — writing
 * INTREQ from the interrupt handler drops IPL with no step in between, and the
 * next VERTB would then find Musashi still holding the old level and never
 * re-trigger. Mirror the level after every advance and after every write that
 * can move it.
 */
static void harness_sync_ipl(harness_t *h)
{
    uint8_t ipl = rigel_get_ipl(h->rigel);

    if (ipl == h->last_ipl) return;
    h->last_ipl = ipl;
    m68k_set_irq(ipl);
}

/* Paula's TX FIFO is shallow, so it has to be emptied on every advance rather
 * than once per frame, or a chatty boot log loses bytes. */
static void harness_drain_serial(harness_t *h)
{
    rigel_u8 byte;

    if (h->serial_fn == NULL) return;

    while (rigel_serial_tx_available(h->rigel) &&
           rigel_serial_pop_tx_byte(h->rigel, &byte))
        h->serial_fn(h->serial_opaque, (uint8_t)byte);
}

/*
 * Emit output samples for `cck` colour clocks of chipset time. Paula's level
 * is read after the step, so every output sample inside one step carries that
 * step's value — fine while steps stay short, which they do because each
 * chipset access flushes.
 */
static void harness_advance_audio(harness_t *h, rigel_cycle_t cck)
{
    rigel_audio_sample_t s;

    if (h->audio_fn == NULL || h->audio_step_q16 == 0u) return;

    h->audio_accum_q16 += (uint32_t)(cck << 16);
    if (h->audio_accum_q16 < h->audio_step_q16) return;

    s = rigel_get_audio_sample(h->rigel);
    while (h->audio_accum_q16 >= h->audio_step_q16) {
        h->audio_accum_q16 -= h->audio_step_q16;
        h->audio_fn(h->audio_opaque, (int16_t)s.left, (int16_t)s.right);
    }
}

static void harness_absorb(harness_t *h, rigel_step_result_t r)
{
    h->pending_events |= r.events;
    harness_sync_ipl(h);
    harness_drain_serial(h);
}

static void harness_flush(harness_t *h)
{
    int      run;
    int      delta;
    uint64_t scaled;
    uint64_t cck;

    run = m68k_cycles_run();
    delta = run - h->cycles_flushed;
    if (delta <= 0) return;
    h->cycles_flushed = run;

    scaled  = (uint64_t)h->cck_rem + (uint64_t)delta;
    cck     = scaled / 2u;
    h->cck_rem = (uint32_t)(scaled & 1u);

    if (cck != 0u) {
        harness_absorb(h, rigel_step(h->rigel, (rigel_cycle_t)cck));
        harness_advance_audio(h, (rigel_cycle_t)cck);
    }
}

/* -------------------------------------------------------------------------
 * Address decode
 * ------------------------------------------------------------------------- */

/* CIA-A PRA bit 0 drives OVL. While DDRA has bit 0 as input (the reset state)
 * the line floats high and the overlay is on, which is what puts the Kickstart
 * reset vector at address 0. */
static bool harness_overlay(harness_t *h)
{
    /* A hunk image lives in low Chip RAM, which the overlay would shadow. With
     * no Kickstart there is nothing to overlay anyway, and nothing will ever
     * clear OVL through the CIA. */
    if (h->exec_mode) return false;
    if (h->rom_size == 0u) return false;
    if (!(rigel_cia_read(h->rigel, 0u, 0x2u) & 0x01u)) return true;
    return (rigel_cia_read(h->rigel, 0u, 0x0u) & 0x01u) != 0u;
}

/* Byte the overlay serves at low address `addr`. */
static uint8_t harness_overlay_read8(harness_t *h, uint32_t addr)
{
    return h->rom[h->ovl_off + (addr & h->ovl_mask)];
}

/* Byte at a ROM-window address, or 0xFF when the address is in neither. */
static uint8_t harness_rom_read8(harness_t *h, uint32_t addr)
{
    if (addr >= HARNESS_ROM_BASE && h->std_size != 0u)
        return h->rom[h->std_off + (addr & h->std_mask)];

    if (h->ext_size != 0u &&
        addr >= HARNESS_ROM_EXT_BASE &&
        addr < HARNESS_ROM_EXT_BASE + h->ext_size)
        return h->rom[h->ext_off + (addr & h->ext_mask)];

    return 0xFFu;
}

/*
 * Autoconfig arbitration. The bus only lets one unconfigured board answer
 * 0xE80000; the next appears once that one has taken a base or shut up.
 * Memory boards go first so the guest has somewhere to put things before the
 * I/O boards are enumerated.
 */
static bool addr_in_autoconfig(uint32_t a)
{
    return a >= ZORRO_AC_BASE && a < ZORRO_AC_BASE + ZORRO_AC_WINDOW;
}

static uint8_t harness_autoconfig_read(harness_t *h, uint32_t addr)
{
    uint32_t off = addr - ZORRO_AC_BASE;

    if (fastram_pending(h->fastram)) return fastram_ac_read(h->fastram, off);
    if (lide_pending(h->lide))       return lide_ac_read(h->lide, off);
    return 0xFFu;
}

static void harness_autoconfig_write(harness_t *h, uint32_t addr, uint8_t value)
{
    uint32_t off = addr - ZORRO_AC_BASE;

    if (fastram_pending(h->fastram))  { fastram_ac_write(h->fastram, off, value); return; }
    if (lide_pending(h->lide))        { lide_ac_write(h->lide, off, value); }
}

static bool harness_autoconfig_active(harness_t *h)
{
    return fastram_pending(h->fastram) || lide_pending(h->lide);
}

static bool addr_in_custom(uint32_t a) { return a >= 0xDF0000u && a < 0xE00000u; }
static bool addr_in_cia(uint32_t a)    { return a >= 0xA00000u && a < 0xC00000u; }

static uint8_t harness_cia_read8(harness_t *h, uint32_t addr)
{
    uint8_t reg = (uint8_t)((addr >> 8) & 0x0Fu);

    harness_flush(h);

    /* CIA-A answers on the odd byte lane when A12 is low; CIA-B on the even
     * lane when A13 is low. A word access that hits neither reads as open bus. */
    if (!(addr & 0x1000u) && (addr & 1u)) {
        /* Reading ICR acknowledges and clears the CIA's pending interrupts,
         * which can drop IPL with no step in between. */
        uint8_t v = rigel_cia_read(h->rigel, 0u, reg);
        harness_sync_ipl(h);
        return v;
    }
    if (!(addr & 0x2000u) && !(addr & 1u)) {
        uint8_t v = rigel_cia_read(h->rigel, 1u, reg);
        harness_sync_ipl(h);
        return v;
    }
    return 0xFFu;
}

static void harness_cia_write8(harness_t *h, uint32_t addr, uint8_t value)
{
    uint8_t reg = (uint8_t)((addr >> 8) & 0x0Fu);

    harness_flush(h);

    if (!(addr & 0x1000u) && (addr & 1u))
        rigel_cia_write(h->rigel, 0u, reg, value);
    else if (!(addr & 0x2000u) && !(addr & 1u))
        rigel_cia_write(h->rigel, 1u, reg, value);

    /* Unmasking via ICR can raise IPL immediately. */
    harness_sync_ipl(h);
}

/* Every custom-space access funnels through these two, so the flush, the IPL
 * republish and the observer hook cannot be forgotten at one call site. */
static uint16_t harness_custom_read(harness_t *h, uint32_t reg)
{
    uint16_t value;

    harness_flush(h);
    value = rigel_custom_read16(h->rigel, reg);
    if (h->mmio_fn != NULL) h->mmio_fn(h->mmio_opaque, reg, value, false);
    return value;
}

static void harness_custom_write(harness_t *h, uint32_t reg, uint16_t value)
{
    harness_flush(h);
    rigel_custom_write16(h->rigel, reg, value);
    if (h->mmio_fn != NULL) h->mmio_fn(h->mmio_opaque, reg, value, true);
    /* INTREQ / INTENA move IPL the instant they are written. */
    harness_sync_ipl(h);
}

/* Unified byte read. Word/long accesses compose from this so every region
 * decodes in exactly one place. */
static uint8_t harness_read8(harness_t *h, uint32_t addr)
{
    addr &= 0x00FFFFFFu;

    if (addr < h->chip_ram_size) {
        if (harness_overlay(h))
            return harness_overlay_read8(h, addr);
        return h->chip_ram[addr];
    }

    if (fastram_owns(h->fastram, addr))
        return (uint8_t)fastram_read(h->fastram, addr, 1);

    if (lide_owns(h->lide, addr))
        return (uint8_t)lide_read(h->lide, addr, 1);

    if (addr_in_autoconfig(addr) && harness_autoconfig_active(h))
        return harness_autoconfig_read(h, addr);

    if (addr_in_cia(addr))
        return harness_cia_read8(h, addr);

    if (h->slow_ram_size != 0u &&
        addr >= HARNESS_SLOW_RAM_BASE &&
        addr < HARNESS_SLOW_RAM_BASE + h->slow_ram_size)
        return h->slow_ram[addr - HARNESS_SLOW_RAM_BASE];

    if (addr_in_custom(addr)) {
        uint16_t w = harness_custom_read(h, addr & 0x1FEu);
        return (uint8_t)((addr & 1u) ? (w & 0xFFu) : (w >> 8));
    }

    return harness_rom_read8(h, addr);
}

static void harness_write8(harness_t *h, uint32_t addr, uint8_t value)
{
    addr &= 0x00FFFFFFu;

    if (addr < h->chip_ram_size) {
        /* Writes always land in RAM: the overlay only redirects reads. */
        h->chip_ram[addr] = value;
        return;
    }

    if (fastram_owns(h->fastram, addr)) {
        fastram_write(h->fastram, addr, value, 1);
        return;
    }

    if (lide_owns(h->lide, addr)) {
        lide_write(h->lide, addr, value, 1);
        return;
    }

    if (addr_in_autoconfig(addr) && harness_autoconfig_active(h)) {
        harness_autoconfig_write(h, addr, value);
        return;
    }

    if (addr_in_cia(addr)) {
        harness_cia_write8(h, addr, value);
        return;
    }

    if (h->slow_ram_size != 0u &&
        addr >= HARNESS_SLOW_RAM_BASE &&
        addr < HARNESS_SLOW_RAM_BASE + h->slow_ram_size) {
        h->slow_ram[addr - HARNESS_SLOW_RAM_BASE] = value;
        return;
    }

    if (addr_in_custom(addr)) {
        /* Custom space is word-only on real hardware; a byte write drives one
         * lane and leaves the other at whatever the bus held. Read-modify-write
         * is the closest a byte-granular host can get. */
        uint32_t reg = addr & 0x1FEu;
        uint16_t cur = harness_custom_read(h, reg);
        if (addr & 1u)
            harness_custom_write(h, reg, (uint16_t)((cur & 0xFF00u) | value));
        else
            harness_custom_write(h, reg,
                                 (uint16_t)((cur & 0x00FFu) | ((uint16_t)value << 8)));
        return;
    }

    /* ROM and unmapped space swallow writes. */
}

static uint16_t harness_read16(harness_t *h, uint32_t addr)
{
    addr &= 0x00FFFFFFu;

    /* Custom registers are natively 16-bit; go straight at them so a read with
     * side effects (INTREQR, DSKDATR, ...) happens once, not twice. */
    if (addr_in_custom(addr))
        return harness_custom_read(h, addr & 0x1FEu);

    if (addr + 1u < h->chip_ram_size) {
        if (harness_overlay(h))
            return (uint16_t)((harness_overlay_read8(h, addr) << 8) |
                              harness_overlay_read8(h, addr + 1u));
        return (uint16_t)((h->chip_ram[addr] << 8) | h->chip_ram[addr + 1u]);
    }

    /* One 16-bit access, not two byte reads: the ATA data port advances its
     * transfer pointer per access, so splitting it corrupts the stream. */
    if (fastram_owns(h->fastram, addr))
        return (uint16_t)fastram_read(h->fastram, addr, 2);

    if (lide_owns(h->lide, addr))
        return (uint16_t)lide_read(h->lide, addr, 2);

    return (uint16_t)(((uint16_t)harness_read8(h, addr) << 8) |
                      harness_read8(h, addr + 1u));
}

static void harness_write16(harness_t *h, uint32_t addr, uint16_t value)
{
    addr &= 0x00FFFFFFu;

    if (addr_in_custom(addr)) {
        harness_custom_write(h, addr & 0x1FEu, value);
        return;
    }

    if (addr + 1u < h->chip_ram_size) {
        h->chip_ram[addr]      = (uint8_t)(value >> 8);
        h->chip_ram[addr + 1u] = (uint8_t)(value & 0xFFu);
        return;
    }

    if (fastram_owns(h->fastram, addr)) {
        fastram_write(h->fastram, addr, value, 2);
        return;
    }

    if (lide_owns(h->lide, addr)) {
        lide_write(h->lide, addr, value, 2);
        return;
    }

    harness_write8(h, addr,      (uint8_t)(value >> 8));
    harness_write8(h, addr + 1u, (uint8_t)(value & 0xFFu));
}

/* -------------------------------------------------------------------------
 * Emu68 timer control registers (MOVEC 0x0E0 / 0x0E1)
 *
 * Emu68's bare-metal examples time themselves through two control registers
 * it invents. Answering with emulated CPU time rather than the wall clock
 * makes a benchmark measure the machine being emulated, and makes the number
 * the same on every run.
 * ------------------------------------------------------------------------- */

unsigned int rigel_m68k_timer_freq(void)
{
    return g_harness != NULL ? rigel_get_clock_hz(g_harness->rigel) : 7093790u;
}

unsigned int rigel_m68k_timer_ticks(void)
{
    if (g_harness == NULL) return 0u;
    /* Cycles run so far, including the part of the current timeslice. */
    return (unsigned int)(g_harness->cpu_cycles_total +
                          (uint64_t)m68k_cycles_run());
}

/* -------------------------------------------------------------------------
 * Musashi memory callbacks
 * ------------------------------------------------------------------------- */

unsigned int m68k_read_memory_8(unsigned int address)
{
    return harness_read8(g_harness, address);
}

unsigned int m68k_read_memory_16(unsigned int address)
{
    return harness_read16(g_harness, address);
}

unsigned int m68k_read_memory_32(unsigned int address)
{
    return ((unsigned int)harness_read16(g_harness, address) << 16) |
           harness_read16(g_harness, address + 2u);
}

void m68k_write_memory_8(unsigned int address, unsigned int value)
{
    harness_write8(g_harness, address, (uint8_t)value);
}

void m68k_write_memory_16(unsigned int address, unsigned int value)
{
    harness_write16(g_harness, address, (uint16_t)value);
}

void m68k_write_memory_32(unsigned int address, unsigned int value)
{
    harness_write16(g_harness, address,      (uint16_t)(value >> 16));
    harness_write16(g_harness, address + 2u, (uint16_t)value);
}

/*
 * Disassembler reads. Musashi keeps these separate from the execution reads on
 * purpose, and so does the harness: disassembly must never advance the clock,
 * touch a read-sensitive custom register, or move IPL. Memory only.
 */
static uint16_t harness_peek16(harness_t *h, uint32_t addr)
{
    addr &= 0x00FFFFFFu;

    if (addr + 1u < h->chip_ram_size) {
        if (harness_overlay(h))
            return (uint16_t)((harness_overlay_read8(h, addr) << 8) |
                              harness_overlay_read8(h, addr + 1u));
        return (uint16_t)((h->chip_ram[addr] << 8) | h->chip_ram[addr + 1u]);
    }

    if (h->slow_ram_size != 0u &&
        addr >= HARNESS_SLOW_RAM_BASE &&
        addr + 1u < HARNESS_SLOW_RAM_BASE + h->slow_ram_size) {
        uint32_t off = addr - HARNESS_SLOW_RAM_BASE;
        return (uint16_t)((h->slow_ram[off] << 8) | h->slow_ram[off + 1u]);
    }

    /* Fast RAM is plain memory, so peeking it has no side effects. The LIDE
     * board is deliberately left out: its ATA registers advance a transfer
     * pointer when read, which a debugger must never do. */
    if (fastram_owns(h->fastram, addr) && fastram_owns(h->fastram, addr + 1u))
        return (uint16_t)fastram_read(h->fastram, addr, 2);

    if (addr_in_cia(addr) || addr_in_custom(addr)) return 0u;
    if (lide_owns(h->lide, addr)) return 0u;

    return (uint16_t)((harness_rom_read8(h, addr) << 8) |
                      harness_rom_read8(h, addr + 1u));
}

bool harness_overlay_active(harness_t *h)
{
    return h != NULL && harness_overlay(h);
}

uint8_t harness_peek8(harness_t *h, uint32_t addr)
{
    uint16_t w;

    if (h == NULL) return 0u;
    w = harness_peek16(h, addr & ~1u);
    return (uint8_t)((addr & 1u) ? (w & 0xFFu) : (w >> 8));
}

void harness_peek(harness_t *h, uint32_t addr, uint8_t *out, uint32_t len)
{
    uint32_t i;

    if (h == NULL || out == NULL) return;
    for (i = 0; i < len; i++)
        out[i] = harness_peek8(h, addr + i);
}

unsigned int m68k_read_disassembler_16(unsigned int address)
{
    return harness_peek16(g_harness, address);
}

unsigned int m68k_read_disassembler_32(unsigned int address)
{
    return ((unsigned int)harness_peek16(g_harness, address) << 16) |
           harness_peek16(g_harness, address + 2u);
}

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

static void harness_synth_reset_vector(harness_t *h)
{
    uint32_t sp = h->chip_ram_size;
    uint32_t pc = HARNESS_ROM_BASE + 8u;

    h->rom[0] = (uint8_t)(sp >> 24); h->rom[1] = (uint8_t)(sp >> 16);
    h->rom[2] = (uint8_t)(sp >>  8); h->rom[3] = (uint8_t)(sp);
    h->rom[4] = (uint8_t)(pc >> 24); h->rom[5] = (uint8_t)(pc >> 16);
    h->rom[6] = (uint8_t)(pc >>  8); h->rom[7] = (uint8_t)(pc);
}

harness_t *harness_create(void)
{
    return harness_create_ex(NULL);
}

harness_t *harness_create_ex(const harness_config_t *cfg)
{
    harness_config_t defaults;
    harness_t *h;

    if (cfg == NULL) {
        memset(&defaults, 0, sizeof(defaults));
        cfg = &defaults;
    }

    h = (harness_t *)calloc(1, sizeof(*h));
    if (h == NULL) return NULL;

    h->chip_ram_size = cfg->chip_ram_size ? cfg->chip_ram_size : HARNESS_CHIP_RAM_SIZE;
    h->slow_ram_size = cfg->slow_ram_size;
    h->rom_size      = HARNESS_ROM_SIZE;
    h->std_off = 0u; h->std_size = HARNESS_ROM_SIZE; h->std_mask = HARNESS_ROM_SIZE - 1u;
    h->ext_off = 0u; h->ext_size = 0u;              h->ext_mask = 0u;
    h->ovl_off = 0u; h->ovl_mask = HARNESS_ROM_SIZE - 1u;

    h->chip_ram = (uint8_t *)calloc(1, h->chip_ram_size);
    h->rom      = (uint8_t *)calloc(1, HARNESS_ROM_MAX);
    if (h->slow_ram_size != 0u)
        h->slow_ram = (uint8_t *)calloc(1, h->slow_ram_size);

    if (h->chip_ram == NULL || h->rom == NULL ||
        (h->slow_ram_size != 0u && h->slow_ram == NULL)) {
        harness_destroy(h);
        return NULL;
    }

    h->config.chip_ram.read16  = chip_ram_read16;
    h->config.chip_ram.write16 = chip_ram_write16;
    h->config.chip_ram.opaque  = h;
    h->config.chip_ram_size    = h->chip_ram_size;
    h->config.video_std        = cfg->video_std;
    h->config.chipset_model    = cfg->chipset_model;
    h->config.cycle_exact      = cfg->cycle_exact;
    if (!cfg->trace_events)
        h->config.log_event_fn = harness_log_event_sink;
    h->config.serial.tx_instant = !cfg->serial_slow;

    if (cfg->want_framebuffer) {
        h->framebuffer = (uint32_t *)calloc(HARNESS_MAX_WIDTH * HARNESS_MAX_HEIGHT,
                                            sizeof(uint32_t));
        if (h->framebuffer == NULL) {
            harness_destroy(h);
            return NULL;
        }
        h->config.pixel_format          = RIGEL_PIXEL_RGBA8888;
        h->config.framebuffer.pixels    = h->framebuffer;
        h->config.framebuffer.width     = HARNESS_MAX_WIDTH;
        h->config.framebuffer.height    = HARNESS_MAX_HEIGHT;
        h->config.framebuffer.pitch     = HARNESS_MAX_WIDTH * sizeof(uint32_t);
        h->config.framebuffer.format    = RIGEL_PIXEL_RGBA8888;
        h->config.framebuffer.little_endian = true;
    }

    h->rigel = rigel_create(&h->config);
    if (h->rigel == NULL) {
        harness_destroy(h);
        return NULL;
    }

    harness_synth_reset_vector(h);

    g_harness = h;
    m68k_init();
    m68k_set_cpu_type(cfg->cpu_type ? (unsigned int)cfg->cpu_type
                                    : (unsigned int)M68K_CPU_TYPE_68000);
    m68k_pulse_reset();
    h->last_pc = m68k_get_reg(NULL, M68K_REG_PC);

    return h;
}

void harness_destroy(harness_t *h)
{
    int i;

    if (h == NULL) return;
    if (h->rigel != NULL) rigel_destroy(h->rigel);
    for (i = 0; i < 4; i++) free(h->adf[i]);
    free(h->framebuffer);
    free(h->slow_ram);
    free(h->rom);
    free(h->chip_ram);
    if (h->lide != NULL) lide_destroy(h->lide);
    if (h->fastram != NULL) fastram_destroy(h->fastram);
    if (g_harness == h) g_harness = NULL;
    free(h);
}

bool harness_attach_fastram(harness_t *h, uint32_t megabytes)
{
    if (h == NULL || h->fastram != NULL) return false;
    h->fastram = fastram_create(megabytes);
    return h->fastram != NULL;
}

bool harness_attach_lide(harness_t *h, const char *rom_path)
{
    if (h == NULL || rom_path == NULL) return false;
    if (h->lide != NULL) return true;
    h->lide = lide_create(rom_path);
    return h->lide != NULL;
}

bool harness_attach_hdf(harness_t *h, const char *path)
{
    return h != NULL && h->lide != NULL && lide_attach_hdf(h->lide, path);
}

bool harness_attach_iso(harness_t *h, const char *path)
{
    return h != NULL && h->lide != NULL && lide_attach_iso(h->lide, path);
}

bool harness_load_odfs(harness_t *h, const char *path)
{
    return h != NULL && h->lide != NULL && lide_load_odfs(h->lide, path);
}

void harness_load_rom(harness_t *h, const uint8_t *data, uint32_t size)
{
    if (h == NULL || data == NULL) return;
    if (size > HARNESS_ROM_SIZE - 8u) size = HARNESS_ROM_SIZE - 8u;
    memcpy(h->rom + 8, data, size);
}

bool harness_load_kickstart(harness_t *h, const uint8_t *data, uint32_t size)
{
    if (h == NULL || data == NULL) return false;
    if (size != 256u * 1024u && size != 512u * 1024u && size != 1024u * 1024u)
        return false;

    memset(h->rom, 0, HARNESS_ROM_MAX);
    memcpy(h->rom, data, size);
    h->rom_size = size;

    if (size == HARNESS_ROM_MAX) {
        /* 1 MB: extended ROM first, standard ROM second. */
        uint32_t half = size / 2u;
        h->ext_off = 0u;    h->ext_size = half; h->ext_mask = half - 1u;
        h->std_off = half;  h->std_size = half; h->std_mask = half - 1u;
        /* The extended half is what the overlay serves at address 0 — that is
         * where a 1 MB AROS image keeps the code the reset vector runs. */
        h->ovl_off = h->ext_off;
        h->ovl_mask = h->ext_mask;
    } else {
        /* A 256K image mirrors through the 512K window, so 0xF80000 and
         * 0xFC0000 both land on offset 0 — what a Kickstart 1.3 machine sees. */
        h->ext_off = h->ext_size = h->ext_mask = 0u;
        h->std_off = 0u; h->std_size = size; h->std_mask = size - 1u;
        h->ovl_off = 0u; h->ovl_mask = size - 1u;
    }

    m68k_pulse_reset();
    h->last_pc = m68k_get_reg(NULL, M68K_REG_PC);
    h->pc_stall_steps = 0;
    return true;
}

/* Guest-memory accessors for the hunk loader. These go through the same byte
 * paths the CPU uses, so a segment placed in Fast RAM lands in the board. */
static void hunk_write8(void *ctx, uint32_t addr, uint8_t value)
{
    harness_write8((harness_t *)ctx, addr, value);
}

static uint8_t hunk_read8(void *ctx, uint32_t addr)
{
    return harness_read8((harness_t *)ctx, addr);
}

bool harness_load_hunk(harness_t *h, const uint8_t *data, uint32_t size,
                       uint32_t fb_width, uint32_t fb_height,
                       char *err, size_t err_len)
{
    hunk_image_t img;
    uint32_t load_addr, limit, sp;
    uint32_t fb, pitch, fb_bytes;

    if (h == NULL || data == NULL) return false;

    if (h->fastram != NULL) {
        /* Nothing will run autoconfig without a Kickstart, so place the board
         * where a guest would have put it and mark it configured. */
        fastram_force_configure(h->fastram, 0x200000u);
        load_addr = fastram_base(h->fastram);
        limit     = load_addr + fastram_size(h->fastram);
    } else {
        /* Leave the low 4 KB alone: that is the exception vector table, and a
         * program that installs handlers expects to own it. */
        load_addr = 0x1000u;
        limit     = h->chip_ram_size;
    }

    if (!hunk_load(data, size, load_addr, limit, h,
                   hunk_write8, hunk_read8, &img, err, err_len))
        return false;

    /*
     * With the overlay off the CPU reads its reset vector straight out of Chip
     * RAM, so write it there: stack below the image when it sits low, PC at
     * the first hunk.
     */
    h->exec_mode = true;
    sp = (img.base >= 0x1000u && img.base < h->chip_ram_size)
        ? h->chip_ram_size   /* image is low: stack at the top of Chip RAM */
        : h->chip_ram_size;
    /*
     * The entry point is a C function: it ends in `rts`, so it needs a return
     * address. Give it one pointing at a branch-to-self, which parks the CPU
     * when the program finishes instead of returning into stack garbage.
     */
    h->exec_exit = HARNESS_EXEC_EXIT_ADDR;
    h->chip_ram[h->exec_exit]      = 0x60u;   /* bra.s * */
    h->chip_ram[h->exec_exit + 1u] = 0xFEu;

    sp -= 4u;
    h->chip_ram[sp]      = (uint8_t)(h->exec_exit >> 24);
    h->chip_ram[sp + 1u] = (uint8_t)(h->exec_exit >> 16);
    h->chip_ram[sp + 2u] = (uint8_t)(h->exec_exit >>  8);
    h->chip_ram[sp + 3u] = (uint8_t)(h->exec_exit);

    h->chip_ram[0] = (uint8_t)(sp >> 24); h->chip_ram[1] = (uint8_t)(sp >> 16);
    h->chip_ram[2] = (uint8_t)(sp >>  8); h->chip_ram[3] = (uint8_t)(sp);
    h->chip_ram[4] = (uint8_t)(img.entry >> 24);
    h->chip_ram[5] = (uint8_t)(img.entry >> 16);
    h->chip_ram[6] = (uint8_t)(img.entry >>  8);
    h->chip_ram[7] = (uint8_t)(img.entry);

    /*
     * Place the framebuffer above the image, and leave a gap for the stack:
     * the entry point is called, not jumped to, and it pushes.
     */
    pitch    = fb_width * 2u;            /* RGB565 */
    fb_bytes = pitch * fb_height;
    fb       = (img.end + 0x10000u + 31u) & ~31u;

    if (fb + fb_bytes > limit) {
        if (err != NULL)
            snprintf(err, err_len,
                     "no room for a %ux%u framebuffer after the image "
                     "(needs %u bytes at %08x, limit %08x)",
                     fb_width, fb_height, fb_bytes, fb, limit);
        return false;
    }

    {
        uint32_t i;
        for (i = 0; i < fb_bytes; i++) harness_write8(h, fb + i, 0u);
    }

    h->exec_fb       = fb;
    h->exec_fb_w     = fb_width;
    h->exec_fb_h     = fb_height;
    h->exec_fb_pitch = pitch;

    m68k_pulse_reset();

    /* The Emu68 entry convention. Without these the example dereferences a
     * garbage framebuffer pointer and writes over memory until it derails. */
    m68k_set_reg(M68K_REG_D0, pitch);
    m68k_set_reg(M68K_REG_A0, fb);
    m68k_set_reg(M68K_REG_D1, fb_width);
    m68k_set_reg(M68K_REG_D2, fb_height);

    h->last_pc = m68k_get_reg(NULL, M68K_REG_PC);
    h->pc_stall_steps = 0;

    printf("[HUNK] %u hunks at %08x..%08x, entry %08x, sp %08x\n",
           img.hunk_count, img.base, img.end, img.entry, sp);
    printf("[HUNK] framebuffer %ux%u RGB565 at %08x (pitch %u)\n",
           fb_width, fb_height, fb, pitch);
    printf("[HUNK] returns to %08x when it finishes\n", h->exec_exit);
    return true;
}

bool harness_exec_framebuffer(const harness_t *h, uint32_t *addr,
                              uint32_t *width, uint32_t *height,
                              uint32_t *pitch)
{
    if (h == NULL || !h->exec_mode || h->exec_fb == 0u) return false;
    if (addr)   *addr   = h->exec_fb;
    if (width)  *width  = h->exec_fb_w;
    if (height) *height = h->exec_fb_h;
    if (pitch)  *pitch  = h->exec_fb_pitch;
    return true;
}

bool harness_insert_adf(harness_t *h, rigel_floppy_drive_id_t drive,
                        const uint8_t *data, uint32_t size)
{
    uint8_t *copy;

    if (h == NULL || data == NULL || size == 0u) return false;
    if ((unsigned)drive >= 4u) return false;

    copy = (uint8_t *)malloc(size);
    if (copy == NULL) return false;
    memcpy(copy, data, size);

    if (rigel_floppy_insert(h->rigel, drive, copy, size) != RIGEL_STATUS_OK) {
        free(copy);
        return false;
    }

    /* Rigel reads through the caller's buffer, so the harness keeps it alive
     * for as long as the disk is in the drive. */
    free(h->adf[drive]);
    h->adf[drive] = copy;
    return true;
}

/* -------------------------------------------------------------------------
 * Stepping
 * ------------------------------------------------------------------------- */

/* CPU cycles to run before Rigel next needs attention. Rigel deadlines are in
 * CCK, so double, minus one if the integrator is already carrying an odd
 * cycle. */
static uint32_t harness_quantum_cpu_cycles(harness_t *h, uint32_t max_cpu_cycles)
{
    rigel_cycle_t now      = rigel_get_time(h->rigel);
    rigel_cycle_t deadline = rigel_get_next_observable_deadline(h->rigel);
    uint64_t      cck      = (deadline > now) ? (uint64_t)(deadline - now) : 1u;
    uint64_t      cpu      = cck * 2u;

    if (h->cck_rem != 0u && cpu > 1u) cpu -= 1u;
    if (max_cpu_cycles != 0u && cpu > max_cpu_cycles) cpu = max_cpu_cycles;

    return cpu != 0u ? (uint32_t)cpu : 1u;
}

uint32_t harness_step(harness_t *h, uint32_t max_cpu_cycles)
{
    uint32_t budget;
    int      consumed;
    uint32_t pc;
    uint32_t events;

    if (h == NULL) return 0;

    h->pending_events = 0;
    budget = harness_quantum_cpu_cycles(h, max_cpu_cycles);

    h->cycles_flushed = 0;
    consumed = m68k_execute((int)budget);
    if (consumed < 0) consumed = 0;

    /* Mid-instruction chipset accesses already flushed part of this timeslice;
     * only what they left behind is still owed to Rigel. */
    {
        int leftover = consumed - h->cycles_flushed;
        if (leftover > 0) {
            uint64_t scaled = (uint64_t)h->cck_rem + (uint64_t)leftover;
            uint64_t cck    = scaled / 2u;
            h->cck_rem = (uint32_t)(scaled & 1u);
            if (cck != 0u) {
                harness_absorb(h, rigel_step(h->rigel, (rigel_cycle_t)cck));
                harness_advance_audio(h, (rigel_cycle_t)cck);
            }
        }
    }

    h->cpu_cycles_total += (uint64_t)consumed;

    pc = m68k_get_reg(NULL, M68K_REG_PC);
    if (pc == h->last_pc && !(h->pending_events & RIGEL_EVENT_IRQ_CHANGED)) {
        if (h->pc_stall_steps < HARNESS_HALT_STEPS) h->pc_stall_steps++;
    } else {
        h->pc_stall_steps = 0;
        h->last_pc = pc;
    }

    events = h->pending_events;
    h->pending_events = 0;
    return events;
}

void harness_run(harness_t *h, bool (*done)(harness_t *), uint64_t max_cycles)
{
    if (h == NULL) return;

    while (!done(h) && h->cpu_cycles_total < max_cycles)
        (void)harness_step(h, 0u);
}

/* -------------------------------------------------------------------------
 * Accessors
 * ------------------------------------------------------------------------- */

void harness_set_mmio_sink(harness_t *h, harness_mmio_fn fn, void *opaque)
{
    if (h == NULL) return;
    h->mmio_fn = fn;
    h->mmio_opaque = opaque;
}

void harness_set_audio_sink(harness_t *h, harness_audio_fn fn, void *opaque,
                            uint32_t rate_hz)
{
    if (h == NULL || rate_hz == 0u) return;

    h->audio_fn = fn;
    h->audio_opaque = opaque;
    h->audio_rate = rate_hz;
    h->audio_accum_q16 = 0u;

    /*
     * Rigel counts CCK; rigel_get_clock_hz reports the CPU-side rate, which is
     * twice that. One output sample is therefore (clock/2)/rate CCK.
     */
    {
        uint32_t cck_hz = rigel_get_clock_hz(h->rigel) / 2u;
        h->audio_step_q16 = (uint32_t)(((uint64_t)cck_hz << 16) / rate_hz);
    }
}

void harness_set_serial_sink(harness_t *h, harness_serial_fn fn, void *opaque)
{
    if (h == NULL) return;
    h->serial_fn = fn;
    h->serial_opaque = opaque;
}

void harness_serial_send(harness_t *h, uint8_t byte)
{
    if (h != NULL) rigel_serial_receive_byte(h->rigel, byte);
}

RigelContext *harness_rigel(harness_t *h) { return h ? h->rigel : NULL; }
uint64_t      harness_cpu_cycles(const harness_t *h) { return h ? h->cpu_cycles_total : 0; }
uint8_t      *harness_chip_ram(harness_t *h) { return h ? h->chip_ram : NULL; }
uint32_t      harness_chip_ram_size(const harness_t *h) { return h ? h->chip_ram_size : 0; }
uint32_t     *harness_framebuffer(harness_t *h) { return h ? h->framebuffer : NULL; }
uint32_t      harness_framebuffer_pitch(const harness_t *h)
{
    (void)h;
    return HARNESS_MAX_WIDTH * (uint32_t)sizeof(uint32_t);
}

bool harness_cpu_halted(const harness_t *h)
{
    return h != NULL && h->pc_stall_steps >= HARNESS_HALT_STEPS;
}
