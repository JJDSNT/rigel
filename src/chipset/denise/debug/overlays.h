#ifndef RIGEL_DENISE_OVERLAYS_H
#define RIGEL_DENISE_OVERLAYS_H

#include <stdbool.h>
#include "rigel/rigel_types.h"

/* Debug overlays — optional visual aids rendered on top of Denise output.
 *
 * Overlays are applied after the normal pixel pipeline and are purely for
 * development/debugging. They should compile to nothing in release builds
 * (controlled by RIGEL_DEBUG_OVERLAYS compile flag).
 *
 * Available overlays:
 *   - Display window boundary (DIWSTRT/DIWSTOP edges)
 *   - Sprite bounding boxes (per sprite, colour-coded)
 *   - Layer highlight (dim all but selected layer)
 *   - DDF/fetch region markers */

typedef struct overlay_config {
    bool enabled;
    bool show_display_window;
    bool show_sprite_bounds;
    bool show_ddf_region;
    int  highlight_layer;     /* -1 = none, 0-7 = sprite N, 8 = pf1, 9 = pf2 */
} overlay_config_t;

void overlay_config_reset(overlay_config_t *cfg);

/* Apply overlays to a pixel; returns modified RGBA. */
rigel_u32 overlay_apply(const overlay_config_t *cfg,
                        rigel_u32 pixel_rgba,
                        rigel_u16 hpos, rigel_u16 vpos);

#endif
