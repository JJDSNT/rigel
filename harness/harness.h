#ifndef RIGEL_HARNESS_H
#define RIGEL_HARNESS_H

#include <stdbool.h>
#include <stdint.h>

#include "rigel/rigel.h"

/*
 * Harness: Musashi (68k) + Rigel (chipset) wired together.
 *
 * Two ways in:
 *   harness_create()      minimal rig for timing tests — synthesised reset
 *                         vector, 512K Chip RAM, no ROM image.
 *   harness_create_ex()   full machine — Kickstart, Slow RAM, CIA decode,
 *                         IPL delivery, framebuffer target, floppy.
 *
 * Memory map (classic OCS/ECS):
 *   0x000000 - chip_ram_size  Chip RAM (ROM overlay while OVL is asserted)
 *   0xA00000 - 0xBFFFFF       CIA-A (A12 low, odd bytes) / CIA-B (A13 low, even)
 *   0xC00000 - 0xC7FFFF       Slow RAM (optional)
 *   0xDFF000 - 0xDFF1FF       Custom registers (Rigel)
 *   0xE00000 - 0xE7FFFF       Extended ROM (1 MB images only)
 *   0xF80000 - 0xFFFFFF       Kickstart ROM (256K images mirror into the window)
 *
 * Time base: Rigel counts colour clocks (CCK, ~3.55 MHz); Musashi counts CPU
 * cycles (~7.09 MHz). The harness carries the /2 conversion and its odd-cycle
 * remainder, so the two clocks never drift.
 */

enum {
    HARNESS_CHIP_RAM_SIZE = 512 * 1024,   /* default, and what harness_create() uses */
    HARNESS_ROM_SIZE      = 512 * 1024,
    HARNESS_ROM_MAX       = 1024 * 1024,  /* 1 MB images split across two windows */
    HARNESS_ROM_BASE      = 0xF80000,
    HARNESS_ROM_EXT_BASE  = 0xE00000,
    HARNESS_SLOW_RAM_BASE = 0xC00000,
    HARNESS_MAX_WIDTH     = 1024,         /* RIGEL_DENISE_MAX_SCANLINE_PIXELS */
    HARNESS_MAX_HEIGHT    = 312           /* PAL */
};

/*
 * Sink for bytes the Amiga transmits through Paula's UART. AROS and DiagROM
 * put their boot log there, so for a development harness this is the single
 * most useful thing coming out of the machine.
 */
typedef void (*harness_serial_fn)(void *opaque, uint8_t byte);

typedef struct harness_config {
    uint32_t chip_ram_size;              /* 0 -> HARNESS_CHIP_RAM_SIZE */
    uint32_t slow_ram_size;              /* 0 -> none */
    int      cpu_type;                   /* M68K_CPU_TYPE_*; 0 -> 68000 */
    rigel_video_std_t     video_std;
    rigel_chipset_model_t chipset_model;
    bool     cycle_exact;
    bool     want_framebuffer;           /* allocate + register a write target */
    /*
     * Rigel's structured log events go to stderr whenever no sink is
     * installed, and the compositor alone emits one per composed line. The
     * harness installs a silent sink unless this is set.
     */
    bool     trace_events;
    /*
     * Queue SERDAT writes immediately instead of pacing them at the programmed
     * baud rate. A host that is not running in real time otherwise spends the
     * whole boot waiting on the UART. Default on.
     */
    bool     serial_slow;
} harness_config_t;

typedef struct harness_t harness_t;

harness_t *harness_create(void);
harness_t *harness_create_ex(const harness_config_t *cfg);
void       harness_destroy(harness_t *h);

/* Load raw 68k code into ROM at offset 8 from HARNESS_ROM_BASE, behind the
 * synthesised reset vector. For test rigs. */
void harness_load_rom(harness_t *h, const uint8_t *data, uint32_t size);

/*
 * Load a Kickstart image and reset the CPU through it. The image supplies its
 * own reset vector; no vector is synthesised.
 *
 * 256K  mirrors through the 0xF80000 window, so 0xF80000 and 0xFC0000 both
 *       reach offset 0 — what a Kickstart 1.x machine sees.
 * 512K  fills the 0xF80000 window.
 * 1 MB  splits: first half is the extended ROM at 0xE00000, second half is the
 *       standard ROM at 0xF80000. This is the AROS layout.
 *
 * Returns false on any other size.
 */
bool harness_load_kickstart(harness_t *h, const uint8_t *data, uint32_t size);

/*
 * Audio sink. Rigel exposes the mixed Paula output as an instantaneous level
 * (rigel_get_audio_sample), so somebody has to resample it to a fixed rate;
 * the harness does that here because it is the side that knows the chipset
 * clock. Samples arrive interleaved stereo at `rate_hz`.
 */
typedef void (*harness_audio_fn)(void *opaque, int16_t left, int16_t right);
void harness_set_audio_sink(harness_t *h, harness_audio_fn fn, void *opaque,
                            uint32_t rate_hz);

/*
 * Observer for custom-register traffic, so a front-end can watch what the
 * guest actually pokes without the harness deciding what is interesting.
 * `reg` is the register offset (DFF000-relative, already masked to 0x1FE).
 */
typedef void (*harness_mmio_fn)(void *opaque, uint32_t reg, uint16_t value,
                                bool is_write);
void harness_set_mmio_sink(harness_t *h, harness_mmio_fn fn, void *opaque);

/* Install the serial sink. Bytes are delivered as Paula transmits them,
 * from inside harness_step. */
void harness_set_serial_sink(harness_t *h, harness_serial_fn fn, void *opaque);

/* Inject a byte into Paula's receive buffer, as if it arrived on RS-232. */
void harness_serial_send(harness_t *h, uint8_t byte);

/*
 * Zorro II Fast RAM. Valid sizes are 1, 2, 4 and 8 MB. Unlike Chip RAM this is
 * not reachable by the chipset, which is the point: code running from it does
 * not contend with DMA for the Chip RAM bus.
 */
bool harness_attach_fastram(harness_t *h, uint32_t megabytes);

/*
 * LIDE — a Zorro II board with LIV2's lide.device, giving the guest a hard
 * disk and a CD-ROM. Attach the board first, then the media. Returns false if
 * the ROM cannot be read, in which case no board is presented at all rather
 * than one the guest cannot drive.
 *
 * Build the ROM with scripts/build-lide-rom.sh.
 */
bool harness_attach_lide(harness_t *h, const char *rom_path);
bool harness_attach_hdf(harness_t *h, const char *path);
bool harness_attach_iso(harness_t *h, const char *path);
bool harness_load_odfs(harness_t *h, const char *path);

/* Insert an ADF into a drive. Thin wrapper over rigel_floppy_insert that keeps
 * ownership of the image with the harness. */
bool harness_insert_adf(harness_t *h, rigel_floppy_drive_id_t drive,
                        const uint8_t *data, uint32_t size);

/* Run until done() returns true or max_cycles CPU cycles elapse. */
void harness_run(harness_t *h, bool (*done)(harness_t *), uint64_t max_cycles);

/*
 * Advance the machine by up to max_cpu_cycles CPU cycles, stopping early at
 * the next Rigel deadline. Returns the accumulated Rigel events, so a caller
 * driving its own loop can react to FRAME_READY without giving up control.
 */
uint32_t harness_step(harness_t *h, uint32_t max_cpu_cycles);

/* Accessors for assertions in tests and for the front-end. */
RigelContext *harness_rigel(harness_t *h);
uint64_t      harness_cpu_cycles(const harness_t *h);
uint8_t      *harness_chip_ram(harness_t *h);
uint32_t      harness_chip_ram_size(const harness_t *h);

/* Framebuffer written by Denise when want_framebuffer was set. Always
 * HARNESS_MAX_WIDTH x HARNESS_MAX_HEIGHT in RIGEL_PIXEL_RGBA8888 (0x00RRGGBB
 * host-native words); the live window is whatever rigel_get_frame reports. */
uint32_t *harness_framebuffer(harness_t *h);
uint32_t  harness_framebuffer_pitch(const harness_t *h);

/*
 * Read memory the way the disassembler does: decoded through the full map but
 * with no side effects — no clock advance, no read-sensitive custom register
 * touched, no IPL movement. Safe to call from monitoring code at any time.
 * Chipset and CIA space read as 0.
 */
uint8_t harness_peek8(harness_t *h, uint32_t addr);

/* True while OVL is asserted, i.e. while low addresses read ROM. Monitoring
 * code needs this to explain what a low-address read actually returned. */
bool harness_overlay_active(harness_t *h);
void    harness_peek(harness_t *h, uint32_t addr, uint8_t *out, uint32_t len);

/* True while the CPU is halted on a double bus fault or STOP with no IRQ. */
bool harness_cpu_halted(const harness_t *h);

#endif
