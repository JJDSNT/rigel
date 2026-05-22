#include "raster.h"

/* TODO(timing): implement raster configuration and window tests */

void raster_reset(raster_config_t *r, agnus_video_std_t std)
{
    (void)r;
    (void)std;
    /* TODO */
}

void raster_set_diwstrt(raster_config_t *r, rigel_u16 val) { r->diwstrt = val; }
void raster_set_diwstop(raster_config_t *r, rigel_u16 val) { r->diwstop = val; }
void raster_set_ddfstrt(raster_config_t *r, rigel_u16 val) { r->ddfstrt = val; }
void raster_set_ddfstop(raster_config_t *r, rigel_u16 val) { r->ddfstop = val; }

bool raster_in_display_window(const raster_config_t *r, rigel_u16 hpos, rigel_u16 vpos)
{
    (void)r;
    (void)hpos;
    (void)vpos;
    /* TODO */
    return false;
}

bool raster_in_fetch_window(const raster_config_t *r, rigel_u16 hpos)
{
    (void)r;
    (void)hpos;
    /* TODO */
    return false;
}
