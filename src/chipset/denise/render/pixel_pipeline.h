#ifndef RIGEL_DENISE_PIXEL_PIPELINE_H
#define RIGEL_DENISE_PIXEL_PIPELINE_H

#include <stdint.h>
#include "rigel/rigel_types.h"

/* Pixel pipeline — the core of Denise.
 *
 * For each pixel clock Denise:
 *   1. Collects 6 bitplane bits (shifted from latched fetch words)
 *   2. Applies video mode: normal, HAM, EHB, dual playfield
 *   3. Composites sprites using priority rules (BPLCON2)
 *   4. Resolves final color index → RGB via palette
 *
 * This stage is stateless per-pixel given fixed inputs; the callers own the
 * latching and shifting. The pipeline is designed to be the hot path — keep
 * everything inline-friendly and avoid branching on rare cases. */

typedef struct denise_pixel_inputs {
    uint8_t  plane_bits;       /* 6 active bitplane bits, bit N = plane N         */
    uint8_t  sprite_pixel[8];  /* per-sprite color index; 0 = transparent         */
    uint8_t  bplcon0_bpu;      /* active plane count (0-6)                        */
    uint16_t bplcon2;          /* priority register                               */
    uint16_t mode_flags;       /* RIGEL_DENISE_MODE_* bitmask                     */
    uint32_t prev_rgb;         /* previous pixel RGB (for HAM hold)               */
} denise_pixel_inputs_t;

/* Run the full pipeline; returns final RGBA pixel (alpha = 0xFF). */
rigel_u32 denise_pixel_pipeline_run(const denise_pixel_inputs_t *in,
                                    const rigel_u32 palette[32]);

#endif
