#include "harness.h"

#include <stdlib.h>
#include <string.h>

#include "m68k.h"
#include "rigel/rigel.h"
#include "rigel/rigel_bus.h"
#include "rigel/rigel_time.h"

/* Global pointer used by Musashi callbacks (Musashi has no userdata slot). */
static harness_t *g_harness;

struct harness_t {
    RigelContext *rigel;
    rigel_config_t config;

    uint8_t chip_ram[HARNESS_CHIP_RAM_SIZE];
    uint8_t rom[HARNESS_ROM_SIZE];

    uint64_t cpu_cycles_total;
};

/* -------------------------------------------------------------------------
 * Chip RAM callbacks wired into rigel_config_t
 * ------------------------------------------------------------------------- */

static uint16_t chip_ram_read16(void *opaque, uint32_t addr)
{
    harness_t *h = (harness_t *)opaque;
    if (addr + 1 >= HARNESS_CHIP_RAM_SIZE) return 0;
    return (uint16_t)((h->chip_ram[addr] << 8) | h->chip_ram[addr + 1]);
}

static void chip_ram_write16(void *opaque, uint32_t addr, uint16_t value)
{
    harness_t *h = (harness_t *)opaque;
    if (addr + 1 >= HARNESS_CHIP_RAM_SIZE) return;
    h->chip_ram[addr]     = (uint8_t)(value >> 8);
    h->chip_ram[addr + 1] = (uint8_t)(value & 0xFF);
}

/* -------------------------------------------------------------------------
 * Musashi memory callbacks
 * ------------------------------------------------------------------------- */

unsigned int m68k_read_memory_8(unsigned int address)
{
    harness_t *h = g_harness;

    if (address < HARNESS_CHIP_RAM_SIZE) {
        /* TODO: honour cpu_would_stall for bus contention */
        return h->chip_ram[address];
    }

    if (address >= HARNESS_ROM_BASE) {
        uint32_t offset = address - HARNESS_ROM_BASE;
        if (offset < HARNESS_ROM_SIZE) return h->rom[offset];
    }

    if (address >= 0xDFF000 && address < 0xDFF200) {
        uint32_t reg = address & 0x1FE;
        return (uint8_t)(rigel_custom_read16(h->rigel, reg) >> 8);
    }

    return 0;
}

unsigned int m68k_read_memory_16(unsigned int address)
{
    harness_t *h = g_harness;

    if (address < HARNESS_CHIP_RAM_SIZE) {
        /* TODO: honour cpu_would_stall */
        return (unsigned int)chip_ram_read16(h, address);
    }

    if (address >= HARNESS_ROM_BASE) {
        uint32_t offset = address - HARNESS_ROM_BASE;
        if (offset + 1 < HARNESS_ROM_SIZE)
            return (unsigned int)((h->rom[offset] << 8) | h->rom[offset + 1]);
    }

    if (address >= 0xDFF000 && address < 0xDFF200) {
        uint32_t reg = address & 0x1FE;
        return (unsigned int)rigel_custom_read16(h->rigel, reg);
    }

    return 0;
}

unsigned int m68k_read_memory_32(unsigned int address)
{
    return (m68k_read_memory_16(address) << 16) | m68k_read_memory_16(address + 2);
}

void m68k_write_memory_8(unsigned int address, unsigned int value)
{
    harness_t *h = g_harness;

    if (address < HARNESS_CHIP_RAM_SIZE) {
        h->chip_ram[address] = (uint8_t)value;
        return;
    }

    if (address >= 0xDFF000 && address < 0xDFF200) {
        /* byte writes to custom space: write to even word, mask high or low */
        uint32_t reg = address & 0x1FE;
        uint16_t cur = rigel_custom_read16(h->rigel, reg);
        if (address & 1)
            rigel_custom_write16(h->rigel, reg, (cur & 0xFF00) | (value & 0xFF));
        else
            rigel_custom_write16(h->rigel, reg, (cur & 0x00FF) | ((value & 0xFF) << 8));
    }
}

void m68k_write_memory_16(unsigned int address, unsigned int value)
{
    harness_t *h = g_harness;

    if (address < HARNESS_CHIP_RAM_SIZE) {
        chip_ram_write16(h, address, (uint16_t)value);
        return;
    }

    if (address >= 0xDFF000 && address < 0xDFF200) {
        uint32_t reg = address & 0x1FE;
        rigel_custom_write16(h->rigel, reg, (uint16_t)value);
    }
}

void m68k_write_memory_32(unsigned int address, unsigned int value)
{
    m68k_write_memory_16(address,     (value >> 16) & 0xFFFF);
    m68k_write_memory_16(address + 2, value & 0xFFFF);
}

/* -------------------------------------------------------------------------
 * Harness lifecycle
 * ------------------------------------------------------------------------- */

harness_t *harness_create(void)
{
    harness_t *h = (harness_t *)calloc(1, sizeof(*h));
    if (h == NULL) return NULL;

    h->config.chip_ram.read16  = chip_ram_read16;
    h->config.chip_ram.write16 = chip_ram_write16;
    h->config.chip_ram.opaque  = h;

    h->rigel = rigel_create(&h->config);
    if (h->rigel == NULL) {
        free(h);
        return NULL;
    }

    /* Reset vector: SP at top of chip RAM, PC at ROM base + 8 */
    uint32_t sp = HARNESS_CHIP_RAM_SIZE;
    uint32_t pc = HARNESS_ROM_BASE + 8;
    h->rom[0] = (uint8_t)(sp >> 24); h->rom[1] = (uint8_t)(sp >> 16);
    h->rom[2] = (uint8_t)(sp >>  8); h->rom[3] = (uint8_t)(sp);
    h->rom[4] = (uint8_t)(pc >> 24); h->rom[5] = (uint8_t)(pc >> 16);
    h->rom[6] = (uint8_t)(pc >>  8); h->rom[7] = (uint8_t)(pc);

    g_harness = h;
    m68k_init();
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    m68k_pulse_reset();

    return h;
}

void harness_destroy(harness_t *h)
{
    if (h == NULL) return;
    rigel_destroy(h->rigel);
    if (g_harness == h) g_harness = NULL;
    free(h);
}

void harness_load_rom(harness_t *h, const uint8_t *data, uint32_t size)
{
    if (h == NULL || data == NULL) return;
    if (size > HARNESS_ROM_SIZE - 8) size = HARNESS_ROM_SIZE - 8;
    memcpy(h->rom + 8, data, size);
}

void harness_run(harness_t *h, bool (*done)(harness_t *), uint64_t max_cycles)
{
    if (h == NULL) return;

    while (!done(h) && h->cpu_cycles_total < max_cycles) {
        rigel_cycle_t deadline = rigel_get_next_deadline(h->rigel);
        rigel_cycle_t now      = rigel_get_time(h->rigel);
        int slots              = (int)(deadline - now);

        if (slots <= 0) slots = 1;

        int consumed = m68k_execute(slots);
        h->cpu_cycles_total += (uint64_t)consumed;

        rigel_step_result_t r = rigel_step_until(h->rigel, now + (rigel_cycle_t)consumed);

        if (r.events & RIGEL_EVENT_IRQ_CHANGED)
            m68k_set_irq(rigel_get_ipl(h->rigel));
    }
}

RigelContext *harness_rigel(harness_t *h) { return h ? h->rigel : NULL; }
uint64_t      harness_cpu_cycles(const harness_t *h) { return h ? h->cpu_cycles_total : 0; }
uint8_t      *harness_chip_ram(harness_t *h) { return h ? h->chip_ram : NULL; }
