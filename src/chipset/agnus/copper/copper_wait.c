#include "copper_wait.h"

/* WAIT/SKIP format (HRM "Copper Instructions"):
 *   IR1 bits[15:8] = VPOS, bits[7:1] = HPOS, bit 0 = 1
 *   IR2 bits[15:8] = VPMASK, bits[7:1] = HPMASK, bit 0 = 0 (WAIT) or 1 (SKIP) */

void copper_wait_arm(copper_state_t *copper, rigel_u16 ir1, rigel_u16 ir2)
{
    copper->wait_vpos   = (ir1 >> 8) & 0xFFu;
    copper->wait_hpos   = ir1 & 0xFEu;
    copper->wait_vpmask = (ir2 >> 8) & 0xFFu;
    copper->wait_hpmask = ir2 & 0xFEu;
    copper->waiting     = true;
    copper->wait_blitter = (ir2 & 0x8000u) == 0u;
    copper->stopped_until_vbl = (ir1 == 0xffffu && ir2 == 0xfffeu);
}

bool copper_wait_satisfied(const copper_state_t *copper,
                           rigel_u16 hpos, rigel_u16 vpos)
{
    if (!copper->waiting) return true;
    return copper_beam_cmp(vpos, hpos,
                           copper->wait_vpos, copper->wait_hpos,
                           copper->wait_vpmask, copper->wait_hpmask);
}
