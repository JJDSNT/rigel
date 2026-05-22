#ifndef RIGEL_AGNUS_DISPLAY_WINDOW_H
#define RIGEL_AGNUS_DISPLAY_WINDOW_H

#include <stdbool.h>
#include "rigel/rigel_types.h"

/* Display / data-fetch window registers — set by the host via MMIO.
 *
 *   DIWSTRT / DIWSTOP  — define the Denise display enable window
 *   DDFSTRT / DDFSTOP  — define when Agnus issues bitplane DMA fetches
 *
 * These are part of the bitplanes sub-module because they directly
 * govern when bitplane DMA runs. raster.h uses them too for timing output. */

typedef struct display_window {
    rigel_u16 diwstrt;
    rigel_u16 diwstop;
    rigel_u16 ddfstrt;
    rigel_u16 ddfstop;
} display_window_t;

void display_window_reset(display_window_t *w);

void display_window_set_diwstrt(display_window_t *w, rigel_u16 val);
void display_window_set_diwstop(display_window_t *w, rigel_u16 val);
void display_window_set_ddfstrt(display_window_t *w, rigel_u16 val);
void display_window_set_ddfstop(display_window_t *w, rigel_u16 val);

bool display_window_fetch_active(const display_window_t *w, rigel_u16 hpos);

#endif
