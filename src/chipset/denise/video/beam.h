#ifndef RIGEL_DENISE_VIDEO_BEAM_H
#define RIGEL_DENISE_VIDEO_BEAM_H

#include <stdbool.h>
#include "rigel/rigel_types.h"

/* Denise's local beam tracker — pixel-level position within a scanline.
 *
 * Agnus is the authoritative source of (hpos, vpos). Denise maintains a
 * local cursor into the scanline buffer updated each `denise_render_pixel()`
 * call. This is not a second source of truth — it is the render cursor. */

typedef struct denise_beam {
    rigel_u16 hpos;      /* current horizontal pixel position                  */
    rigel_u16 vpos;      /* current scanline                                   */
    bool      in_window; /* cached result of display window test at this hpos  */
} denise_beam_t;

void denise_beam_reset(denise_beam_t *b);

/* Synchronise from Agnus beam at the start of each chip cycle */
void denise_beam_sync(denise_beam_t *b, rigel_u16 hpos, rigel_u16 vpos);

/* Advance one pixel clock */
void denise_beam_advance(denise_beam_t *b);

#endif
