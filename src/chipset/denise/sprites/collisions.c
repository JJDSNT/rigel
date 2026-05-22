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
    (void)c;
    (void)sprite_mask;
    (void)pf1_active;
    (void)pf2_active;
    /* TODO(sprites): implement per-pixel collision accumulation */
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
