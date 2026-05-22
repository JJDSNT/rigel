#include "border.h"

void border_reset(border_state_t *b)
{
    b->diwstrt = 0;
    b->diwstop = 0;
}

void border_set_diwstrt(border_state_t *b, rigel_u16 val) { b->diwstrt = val; }
void border_set_diwstop(border_state_t *b, rigel_u16 val) { b->diwstop = val; }

bool border_active(const border_state_t *b, rigel_u16 hpos, rigel_u16 vpos)
{
    rigel_u16 h_start = b->diwstrt & 0xFFu;
    rigel_u16 v_start = (b->diwstrt >> 8) & 0xFFu;
    rigel_u16 h_stop  = b->diwstop & 0xFFu;
    rigel_u16 v_stop  = (b->diwstop >> 8) & 0xFFu;

    /* TODO(video): handle the ECS extended display window (DIWHIGH register) */
    if (vpos < v_start || vpos > v_stop) return true;
    if (hpos < h_start || hpos > h_stop) return true;
    return false;
}
