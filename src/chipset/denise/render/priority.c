#include "priority.h"

/* Priority resolution — HRM "Sprite and Playfield Priority" chapter.
 *
 * sprite_px[i] must be the final palette index for sprite i (0 = transparent).
 * Callers map the 2-bit raw sprite value to a palette slot before calling here.
 *
 * A sprite pair p (= sprite_num / 2) beats a playfield when either:
 *   - the playfield pixel is transparent (pf_color == 0), or
 *   - pair + PFxP < 4  (lower pair number = higher sprite priority).
 * Sprites are tested 0→7 so the lowest-numbered non-transparent sprite wins. */

priority_result_t denise_priority_resolve(
    uint8_t pf_color,
    bool    is_pf2,
    const uint8_t sprite_px[8],
    uint16_t bplcon2)
{
    priority_result_t r = {0, false, 0};
    unsigned pf_prio = is_pf2
        ? ((bplcon2 >> 3) & 0x7u)
        : (bplcon2 & 0x7u);
    int i;

    for (i = 0; i < 8; i++) {
        unsigned pair = (unsigned)i / 2u;
        if (!sprite_px[i]) continue;
        if (pf_color == 0 || pair + pf_prio < 4u) {
            r.from_sprite = true;
            r.sprite_num  = (uint8_t)i;
            r.color_index = sprite_px[i];
            return r;
        }
    }

    r.color_index = pf_color;
    return r;
}
