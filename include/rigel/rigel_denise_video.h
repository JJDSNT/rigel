#ifndef RIGEL_DENISE_VIDEO_H
#define RIGEL_DENISE_VIDEO_H

#include "rigel_denise_types.h"

bool rigel_denise_get_video_desc(const RigelContext *ctx, rigel_denise_video_desc_t *desc);
bool rigel_denise_get_current_scanline(const RigelContext *ctx, rigel_denise_scanline_t *scanline);

#endif
