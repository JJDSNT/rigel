#ifndef RIGEL_AGNUS_BLITTER_TYPES_H
#define RIGEL_AGNUS_BLITTER_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "rigel/rigel_config.h"

/*
 * Rigel Agnus Blitter
 *
 * This module separates:
 *
 *   1. Classic Amiga register facade
 *   2. Frozen semantic BlitCommand
 *   3. Reference software execution
 *   4. Timing / DMA publication
 *
 * The goal is not to force cycle-exact execution internally.
 * Agnus owns observable time; the blitter backend owns computation.
 */

#define BLITTER_CHIP_ADDR_MASK   0x001FFFFEu
#define BLITTER_CHIP_WORD_MASK   0x000FFFFFu

#define BLITTER_INTREQ_BLIT      0x8040u

#define BLITTER_DMACON_DMAEN     0x0200u
#define BLITTER_DMACON_BLTEN     0x0040u
#define BLITTER_DMACON_BLTNZERO  0x2000u
#define BLITTER_DMACON_BLTBUSY   0x4000u

#define BLITTER_CHANNEL_A        0x08u
#define BLITTER_CHANNEL_B        0x04u
#define BLITTER_CHANNEL_C        0x02u
#define BLITTER_CHANNEL_D        0x01u

typedef enum BlitterMode {
    BLITTER_MODE_COPY = 0,
    BLITTER_MODE_LINE = 1
} BlitterMode;

typedef enum BlitterFillMode {
    BLITTER_FILL_NONE = 0,
    BLITTER_FILL_INCLUSIVE,
    BLITTER_FILL_EXCLUSIVE
} BlitterFillMode;

typedef enum BlitterExecState {
    BLITTER_EXEC_IDLE = 0,
    BLITTER_EXEC_PENDING,
    BLITTER_EXEC_RUNNING
} BlitterExecState;

typedef struct BlitterLineState {
    int16_t  error;
    uint16_t pattern;
    uint32_t cpt;
    uint16_t step_index;
    uint8_t  x_shift;
    uint8_t  one_dot_count;
    uint16_t last_cdat;
    uint16_t last_ddat;
    bool     zero;
    bool     sign;
    bool     initialized;
    /* Cycle-exact cadence: each line pixel is a C read + D write = two chip-bus
     * slots. 0 = next granted slot is the read (no pixel commit yet), 1 = the
     * write (commit the pixel). Only consulted when the cycle-exact gate is on. */
    uint8_t  pixel_slot_phase;
} BlitterLineState;

typedef struct BlitterRegs {
    uint16_t bltcon0;
    uint16_t bltcon1;

    uint32_t bltapt;
    uint32_t bltbpt;
    uint32_t bltcpt;
    uint32_t bltdpt;

    int16_t bltamod;
    int16_t bltbmod;
    int16_t bltcmod;
    int16_t bltdmod;

    uint16_t bltadat;
    uint16_t bltbdat;
    uint16_t bltcdat;
    uint16_t bltddat;
    uint16_t bltbhold;

    uint16_t bltafwm;
    uint16_t bltalwm;

    uint16_t bltsize;
    uint16_t bltsizh;
    uint16_t bltsizv;
} BlitterRegs;

typedef struct BlitCommand {
    /*
     * Semantic snapshot frozen at BLTSIZE write time.
     *
     * It is intentionally independent from register storage:
     * the command describes one operation, not a live MMIO view.
     */
    uint32_t apt;
    uint32_t bpt;
    uint32_t cpt;
    uint32_t dpt;

    int16_t amod;
    int16_t bmod;
    int16_t cmod;
    int16_t dmod;

    uint16_t adat;
    uint16_t bdat;
    uint16_t cdat;
    uint16_t bhold;

    uint16_t afwm;
    uint16_t alwm;

    uint16_t width_words;
    uint16_t height_lines;

    uint8_t minterm;
    uint8_t ashift;
    uint8_t bshift;

    bool use_a;
    bool use_b;
    bool use_c;
    bool use_d;

    bool descending;
    bool fill_carry_in;

    uint8_t line_octant;
    uint8_t line_start_bit;
    bool line_single_dot;
    bool line_initial_sign;

    BlitterMode mode;
    BlitterFillMode fill_mode;
} BlitCommand;

typedef struct BlitterResult {
    uint32_t final_apt;
    uint32_t final_bpt;
    uint32_t final_cpt;
    uint32_t final_dpt;

    uint16_t final_adat;
    uint16_t final_bdat;
    uint16_t final_cdat;
    uint16_t final_ddat;
    uint16_t final_bhold;

    bool zero;
} BlitterResult;

typedef rigel_chip_ram_if_t BlitterMemory;

typedef void (*BlitterIrqRaiseFn)(void *opaque, uint16_t mask);

typedef struct BlitterIrqSink {
    void *opaque;
    BlitterIrqRaiseFn raise;
} BlitterIrqSink;

#endif
