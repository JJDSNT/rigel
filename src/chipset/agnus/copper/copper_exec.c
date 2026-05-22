#include "copper_exec.h"

/* TODO(copper): implement MOVE execution (route to agnus_mmio_write / denise_mmio_write)
 *               and SKIP beam comparison */

void copper_exec_move(RigelContext *ctx, rigel_u16 ir1, rigel_u16 ir2)
{
    (void)ctx;
    (void)ir1;
    (void)ir2;
    /* TODO */
}

bool copper_exec_skip_test(rigel_u16 ir1, rigel_u16 ir2,
                           rigel_u16 hpos, rigel_u16 vpos)
{
    (void)ir1;
    (void)ir2;
    (void)hpos;
    (void)vpos;
    /* TODO */
    return false;
}
