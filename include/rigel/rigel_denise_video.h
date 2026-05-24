#ifndef RIGEL_DENISE_VIDEO_H
#define RIGEL_DENISE_VIDEO_H

#include "rigel_denise_types.h"

bool rigel_denise_get_video_desc(const RigelContext *ctx, rigel_denise_video_desc_t *desc);
bool rigel_denise_get_current_scanline(const RigelContext *ctx, rigel_denise_scanline_t *scanline);
bool rigel_get_frame(const RigelContext *ctx, rigel_frame_t *frame);

/* Fill `out` with pixel data for raster line y (0–311 PAL, 0–261 NTSC).
 * `pixels_rgba` points into the internal frame buffer — valid until the next
 * rigel_step that advances past the start of the following frame.
 * Returns false if ctx/out is NULL or y is out of range. */
bool rigel_get_scanline(const RigelContext *ctx, rigel_u16 y, rigel_denise_scanline_t *out);

#endif
