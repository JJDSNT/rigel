#include "copper_wait.h"

/* TODO(copper): implement WAIT arm and satisfaction test.
 * Reference: HRM "Copper Instructions" — WAIT beam comparison with mask. */

void copper_wait_arm(copper_state_t *copper, rigel_u16 ir1, rigel_u16 ir2)
{
    (void)ir2;
    copper->wait_vpos = (ir1 >> 8) & 0xFFu;
    copper->wait_hpos = (ir1 >> 1) & 0x7Fu;
    copper->waiting   = true;
    /* TODO: store mask from IR2 */
}

bool copper_wait_satisfied(const copper_state_t *copper,
                           rigel_u16 hpos, rigel_u16 vpos)
{
    if (!copper->waiting) return true;
    if (vpos < copper->wait_vpos) return false;
    if (vpos > copper->wait_vpos) return true;
    return hpos >= copper->wait_hpos;
}
