#ifndef RIGEL_DENISE_SCANLINE_H
#define RIGEL_DENISE_SCANLINE_H

#include <stdbool.h>
#include "rigel/rigel_types.h"

/* Scanline buffer — owns the RGBA pixel array for one scanline.
 *
 * The compositor writes pixels here as Agnus steps through each CCK.
 * At end-of-line the host reads the buffer via rigel_denise_get_current_scanline().
 *
 * Buffer is intentionally separate from `rigel_denise_output_state_t` so that
 * it can later be double-buffered, zero-copy mapped, or handed to a GPU upload
 * path without changing the chipset logic. */

#define DENISE_SCANLINE_MAX_PIXELS 1024

typedef struct denise_scanline {
    rigel_u32 pixels[DENISE_SCANLINE_MAX_PIXELS];
    rigel_u16 width;      /* active pixel count this scanline              */
    rigel_u16 y;          /* scanline number                               */
    bool      visible;    /* is this a visible (non-vblank) line?          */
    bool      dirty;      /* has at least one pixel been written?          */
} denise_scanline_t;

void denise_scanline_reset(denise_scanline_t *sl);
void denise_scanline_begin(denise_scanline_t *sl, rigel_u16 y, rigel_u16 width, bool visible);
void denise_scanline_put_pixel(denise_scanline_t *sl, rigel_u16 x, rigel_u32 rgba);
void denise_scanline_end(denise_scanline_t *sl);

#endif
