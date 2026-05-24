#include "priority.h"

/* TODO(render): implement full priority resolution.
 * Reference: HRM "Sprite and Playfield Priority" chapter. */

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

    /* Sprite pair p beats PFx when PF is transparent or pair + PFxP < 4.
     * Lower sprite number = higher priority; first non-transparent that beats PF wins. */
    for (int i = 0; i < 8; i++) {
        unsigned pair = (unsigned)i / 2u;
        if (!sprite_px[i]) continue;
        if (pf_color == 0 || pair + pf_prio < 4u) {
            r.from_sprite = true;
            r.sprite_num  = (uint8_t)i;
            r.color_index = (uint8_t)(16u + pair * 4u + sprite_px[i]);
            return r;
        }
    }

    r.color_index = pf_color;
    return r;
}
