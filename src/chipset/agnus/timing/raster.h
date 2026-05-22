#ifndef RIGEL_AGNUS_TIMING_RASTER_H
#define RIGEL_AGNUS_TIMING_RASTER_H

#include <stdbool.h>
#include "rigel/rigel_types.h"
#include "agnus/agnus_config.h"

/* Raster configuration — PAL/NTSC geometry and display window registers.
 * Agnus uses this to decide when bitplane DMA is active and when to trigger
 * vertical blank. Denise reads DIWSTRT/DIWSTOP to gate pixel output. */

typedef struct raster_config {
    agnus_video_std_t std;

    rigel_u16 line_clocks;   /* CCKs per scanline                     */
    rigel_u16 frame_lines;   /* scanlines per frame                   */

    rigel_u16 diwstrt;       /* display window start (VVVVVVVVHHHHHHHH) */
    rigel_u16 diwstop;       /* display window stop                     */
    rigel_u16 ddfstrt;       /* data fetch start (horizontal, /2)       */
    rigel_u16 ddfstop;       /* data fetch stop                         */
} raster_config_t;

void raster_reset(raster_config_t *r, agnus_video_std_t std);

void raster_set_diwstrt(raster_config_t *r, rigel_u16 val);
void raster_set_diwstop(raster_config_t *r, rigel_u16 val);
void raster_set_ddfstrt(raster_config_t *r, rigel_u16 val);
void raster_set_ddfstop(raster_config_t *r, rigel_u16 val);

bool raster_in_display_window(const raster_config_t *r, rigel_u16 hpos, rigel_u16 vpos);
bool raster_in_fetch_window(const raster_config_t *r, rigel_u16 hpos);

#endif
