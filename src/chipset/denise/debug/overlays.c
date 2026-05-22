#include "overlays.h"

void overlay_config_reset(overlay_config_t *cfg)
{
    cfg->enabled             = false;
    cfg->show_display_window = false;
    cfg->show_sprite_bounds  = false;
    cfg->show_ddf_region     = false;
    cfg->highlight_layer     = -1;
}

rigel_u32 overlay_apply(const overlay_config_t *cfg,
                        rigel_u32 pixel_rgba,
                        rigel_u16 hpos, rigel_u16 vpos)
{
    if (!cfg->enabled) return pixel_rgba;

    (void)hpos;
    (void)vpos;

    /* TODO(debug): implement overlay rendering */
    return pixel_rgba;
}
