#include "denise/render/playfield.h"

#include "rigel/rigel_denise_types.h"

void rigel_denise_playfield_update_mode(RigelDenise *denise)
{
    rigel_u16 flags;

    if (denise == NULL) {
        return;
    }

    flags = 0;
    if ((denise->regs.bplcon0 & 0x0400u) != 0) {
        flags |= RIGEL_DENISE_MODE_DUALPF;
    }
    if ((denise->regs.bplcon0 & 0x0800u) != 0) {
        flags |= RIGEL_DENISE_MODE_HAM;
    }
    if ((denise->regs.bplcon0 & 0x0080u) != 0) {
        flags |= RIGEL_DENISE_MODE_EHB;
    }

    denise->debug.active_mode_flags = flags;
}
