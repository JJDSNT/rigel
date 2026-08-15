#ifndef RIGEL_HARNESS_DIAG_H
#define RIGEL_HARNESS_DIAG_H

#include <stdbool.h>
#include <stdint.h>

#include "harness.h"

/*
 * Development instrumentation for the harness.
 *
 * A machine that boots but draws nothing tells you very little on its own.
 * This turns the run into a readable trace: which registers the guest pokes,
 * when DMA and interrupt masks change, where the beam and the CPU are, and
 * what the exception vectors look like.
 */

typedef enum harness_diag_flags {
    HARNESS_DIAG_NONE    = 0,
    HARNESS_DIAG_REGS    = 1u << 0,  /* every custom register write */
    HARNESS_DIAG_DMA     = 1u << 1,  /* DMACON / bitplane pointers / modulos */
    HARNESS_DIAG_IRQ     = 1u << 2,  /* INTENA / INTREQ / IPL */
    HARNESS_DIAG_DISK    = 1u << 3,  /* floppy registers and drive state */
    HARNESS_DIAG_COPPER  = 1u << 4,  /* copper list pointers and control */
    HARNESS_DIAG_BLITTER = 1u << 5,  /* blitter setup and completion */
    HARNESS_DIAG_VIDEO   = 1u << 6,  /* display window, modulos, BPLCON */
    HARNESS_DIAG_CIA     = 1u << 7,  /* overlay and other CIA-driven state */
    HARNESS_DIAG_ALL     = 0xFFu
} harness_diag_flags_t;

/* Parse a comma-separated category list ("dma,irq" or "all").
 * Returns false and leaves *out untouched on an unknown name. */
bool harness_diag_parse(const char *list, uint32_t *out, const char **bad);

/* Names accepted by harness_diag_parse, for a usage message. */
const char *harness_diag_categories(void);

/* Attach to a harness. status_every is the frame interval for the one-line
 * machine summary; 0 disables it. */
void harness_diag_attach(harness_t *h, uint32_t flags, uint32_t status_every);

/* Call once per completed frame. */
void harness_diag_frame(harness_t *h, uint64_t frame);

/* Call when the run ends. */
void harness_diag_summary(harness_t *h, uint64_t frames);

/*
 * CPU tracing, via the Musashi instruction hook that patches/musashi/0001
 * turns on.
 *
 * ring     keep the last `ring` instructions and dump them when the run ends
 *          or the CPU wedges — the usual "where did it actually go" question.
 * pc_lo/hi when hi > lo, also print instructions live while the PC is inside
 *          that window.
 */
void harness_diag_set_cpu_trace(uint32_t ring, uint32_t pc_lo, uint32_t pc_hi,
                                int cpu_type);

/* Dump the instruction ring, most recent last. */
void harness_diag_dump_trace(void);

/* Disassemble `count` instructions starting at `addr`, reading through the
 * harness's side-effect-free peek path. */
void harness_diag_disasm(harness_t *h, uint32_t addr, uint32_t count,
                         int cpu_type);

/* -------------------------------------------------------------------------
 * Memory monitoring
 * ------------------------------------------------------------------------- */

/* Watch a region and report which bytes changed, once per frame. At most
 * HARNESS_DIAG_WATCH_MAX regions. */
#define HARNESS_DIAG_WATCH_MAX 4
bool harness_diag_watch(harness_t *h, uint32_t addr, uint32_t len);

/* Hex-dump a region. `path` NULL writes an annotated dump to stdout;
 * otherwise raw bytes go to that file. */
bool harness_diag_dump_mem(harness_t *h, uint32_t addr, uint32_t len,
                           const char *path);

/* -------------------------------------------------------------------------
 * Execution control
 * ------------------------------------------------------------------------- */

/* Stop the run when the PC reaches `addr`. Requires the instruction hook, so
 * it shares the machinery with harness_diag_set_cpu_trace. */
void harness_diag_set_breakpoint(uint32_t addr, int cpu_type);

/* True once a breakpoint has been hit. */
bool harness_diag_breakpoint_hit(uint32_t *addr_out);

/* Human-readable name for a custom register offset, or NULL. */
const char *harness_diag_reg_name(uint32_t reg);

#endif
