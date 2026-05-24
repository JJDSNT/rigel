#include "collisions.h"

void collision_reset(collision_state_t *c)
{
    c->clxcon = 0;
    c->clxdat = 0;
}

void collision_check_pixel(collision_state_t *c,
                           uint8_t sprite_mask,
                           bool pf1_active, bool pf2_active)
{
    /* CLXCON[5:0] enable sprite pairs for sprite-sprite detection:
     *   bit 0 = pair 0,1 | bit 1 = pair 2,3 | bit 2 = pair 4,5 | bit 3 = pair 6,7 */
    uint8_t en_pairs = (uint8_t)(c->clxcon & 0x0Fu);
    uint8_t en_all   = 0u;
    if (en_pairs & 0x01u) en_all |= 0x03u;
    if (en_pairs & 0x02u) en_all |= 0x0Cu;
    if (en_pairs & 0x04u) en_all |= 0x30u;
    if (en_pairs & 0x08u) en_all |= 0xC0u;

    /* Gate sprite mask by sprite-sprite enable; split into odd/even groups */
    uint8_t active_ss = sprite_mask & en_all;
    uint8_t odd  = (uint8_t)(((uint8_t)(active_ss >> 1)) & 0x55u); /* sprites 1,3,5,7 */
    uint8_t even = (uint8_t)(active_ss & 0x55u);                   /* sprites 0,2,4,6 */

    if (odd  & (uint8_t)(odd  - 1u)) c->clxdat |= (1u << 0); /* odd-odd   */
    if (even & (uint8_t)(even - 1u)) c->clxdat |= (1u << 1); /* even-even */

    /* CLXCON[11:6]: enable sprite-playfield collision (any bit set enables all) */
    if ((c->clxcon >> 6) & 0x3Fu) {
        uint8_t active_pf = sprite_mask; /* use full mask for PF collision */
        uint8_t odd_pf  = (uint8_t)(((uint8_t)(active_pf >> 1)) & 0x55u);
        uint8_t even_pf = (uint8_t)(active_pf & 0x55u);
        if (odd_pf  && pf1_active) c->clxdat |= (1u << 2);
        if (even_pf && pf1_active) c->clxdat |= (1u << 3);
        if (odd_pf  && pf2_active) c->clxdat |= (1u << 4);
        if (even_pf && pf2_active) c->clxdat |= (1u << 5);
    }

    if (pf1_active && pf2_active) c->clxdat |= (1u << 6); /* PF1 vs PF2 */
}

rigel_u16 collision_read_clxdat(collision_state_t *c)
{
    rigel_u16 val = c->clxdat;
    c->clxdat = 0;
    return val;
}

void collision_write_clxcon(collision_state_t *c, rigel_u16 val)
{
    c->clxcon = val;
}
