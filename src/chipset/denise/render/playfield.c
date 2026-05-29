#include "denise/render/playfield.h"

#include "rigel/rigel_denise_types.h"

void rigel_denise_playfield_update_mode(RigelDenise *denise)
{
    rigel_u16 flags;
    rigel_u16 depth;

    if (denise == NULL) {
        return;
    }

    flags = 0;
    depth = (rigel_u16)((denise->regs.bplcon0 >> 12) & 0x7u);
    if ((denise->regs.bplcon0 & 0x8000u) != 0) {
        flags |= RIGEL_DENISE_MODE_HIRES;
    }
    if ((denise->regs.bplcon0 & 0x0400u) != 0) {
        flags |= RIGEL_DENISE_MODE_DUALPF;
    }
    if ((denise->regs.bplcon0 & 0x0800u) != 0) {
        flags |= RIGEL_DENISE_MODE_HAM;
    }
    if ((denise->regs.bplcon0 & 0x0004u) != 0) {
        flags |= RIGEL_DENISE_MODE_LACE;
    }
    if (depth == 6u &&
        (flags & (RIGEL_DENISE_MODE_HAM | RIGEL_DENISE_MODE_DUALPF)) == 0u &&
        (denise->chip_rev != AGNUS_REV_ECS || (denise->regs.bplcon2 & 0x0040u) == 0u)) {
        flags |= RIGEL_DENISE_MODE_EHB;
    }

    denise->debug.active_mode_flags = flags;
}
