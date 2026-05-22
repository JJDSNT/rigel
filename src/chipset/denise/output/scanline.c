#include "scanline.h"

void denise_scanline_reset(denise_scanline_t *sl)
{
    sl->width   = 0;
    sl->y       = 0;
    sl->visible = false;
    sl->dirty   = false;
}

void denise_scanline_begin(denise_scanline_t *sl, rigel_u16 y, rigel_u16 width, bool visible)
{
    sl->y       = y;
    sl->width   = width < DENISE_SCANLINE_MAX_PIXELS ? width : DENISE_SCANLINE_MAX_PIXELS;
    sl->visible = visible;
    sl->dirty   = false;
}

void denise_scanline_put_pixel(denise_scanline_t *sl, rigel_u16 x, rigel_u32 rgba)
{
    if (x >= sl->width) return;
    sl->pixels[x] = rgba;
    sl->dirty = true;
}

void denise_scanline_end(denise_scanline_t *sl)
{
    (void)sl;
    /* TODO(output): notify host / mark frame dirty when last visible line ends */
}
