#include "denise/registers/denise_registers.h"

#include "denise/palette/color_regs.h"
#include "denise/render/playfield.h"
#include "denise/video/display_window.h"
#include "rigel/rigel_custom.h"

enum {
    RIGEL_REG_BPLCON0 = 0x100,
    RIGEL_REG_BPLCON1 = 0x102,
    RIGEL_REG_BPLCON2 = 0x104,
    RIGEL_REG_DIWSTRT = 0x08e,
    RIGEL_REG_DIWSTOP = 0x090,
    RIGEL_REG_COLOR31 = 0x1be
};

bool rigel_denise_registers_owns_reg(rigel_u32 addr)
{
    if (addr >= RIGEL_REG_COLOR00 && addr <= RIGEL_REG_COLOR31) {
        return true;
    }

    switch (addr) {
    case RIGEL_REG_BPLCON0:
    case RIGEL_REG_BPLCON1:
    case RIGEL_REG_BPLCON2:
    case RIGEL_REG_DIWSTRT:
    case RIGEL_REG_DIWSTOP:
        return true;
    default:
        return false;
    }
}

rigel_u16 rigel_denise_registers_read(RigelDenise *denise, rigel_u32 addr)
{
    if (denise == NULL) {
        return 0;
    }

    if (addr >= RIGEL_REG_COLOR00 && addr <= RIGEL_REG_COLOR31) {
        return rigel_denise_color_regs_read(denise, addr);
    }

    switch (addr) {
    case RIGEL_REG_BPLCON0:
        return denise->regs.bplcon0;
    case RIGEL_REG_BPLCON1:
        return denise->regs.bplcon1;
    case RIGEL_REG_BPLCON2:
        return denise->regs.bplcon2;
    case RIGEL_REG_DIWSTRT:
        return denise->regs.diwstrt;
    case RIGEL_REG_DIWSTOP:
        return denise->regs.diwstop;
    default:
        return 0;
    }
}

void rigel_denise_registers_write(RigelDenise *denise, rigel_u32 addr, rigel_u16 value)
{
    if (denise == NULL) {
        return;
    }

    if (addr >= RIGEL_REG_COLOR00 && addr <= RIGEL_REG_COLOR31) {
        rigel_denise_color_regs_write(denise, addr, value);
        return;
    }

    switch (addr) {
    case RIGEL_REG_BPLCON0:
        denise->regs.bplcon0 = value;
        rigel_denise_playfield_update_mode(denise);
        break;
    case RIGEL_REG_BPLCON1:
        denise->regs.bplcon1 = value;
        break;
    case RIGEL_REG_BPLCON2:
        denise->regs.bplcon2 = value;
        break;
    case RIGEL_REG_DIWSTRT:
        denise->regs.diwstrt = value;
        rigel_denise_display_window_update(denise);
        break;
    case RIGEL_REG_DIWSTOP:
        denise->regs.diwstop = value;
        rigel_denise_display_window_update(denise);
        break;
    default:
        break;
    }
}
