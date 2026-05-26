#include "denise/registers/denise_registers.h"

#include "denise/palette/color_regs.h"
#include "denise/render/playfield.h"
#include "denise/sprites/collisions.h"
#include "denise/sprites/sprites.h"
#include "denise/video/display_window.h"
#include "rigel/rigel_custom.h"

enum {
    RIGEL_REG_CLXDAT   = 0x00e,
    RIGEL_REG_CLXCON   = 0x098,
    RIGEL_REG_SPR0POS  = 0x140,
    RIGEL_REG_SPR7DATB = 0x17e
};

bool rigel_denise_registers_owns_reg(rigel_u32 addr)
{
    if (addr >= RIGEL_REG_COLOR00 && addr <= RIGEL_REG_COLOR31) {
        return true;
    }

    if (addr >= RIGEL_REG_SPR0POS && addr <= RIGEL_REG_SPR7DATB && (addr & 1u) == 0u) {
        return true;
    }

    switch (addr) {
    case RIGEL_REG_CLXDAT:
    case RIGEL_REG_CLXCON:
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
    case RIGEL_REG_CLXDAT:
        /* CLXDAT clears on read */
        return collision_read_clxdat(&denise->coll);
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

    /* SPR0POS-SPR7DATB: 8 sprites × {POS,CTL,DATA,DATB} at 0x140-0x17E */
    if (addr >= RIGEL_REG_SPR0POS && addr <= RIGEL_REG_SPR7DATB && (addr & 1u) == 0u) {
        unsigned sp  = (unsigned)((addr - RIGEL_REG_SPR0POS) / 8u);
        unsigned sub = (unsigned)((addr - RIGEL_REG_SPR0POS) % 8u);
        if (sp < DENISE_SPRITE_COUNT) {
            switch (sub) {
            case 0: /* POS */
                denise->sprites.sp[sp].pos = value;
                break;
            case 2: /* CTL */
                denise->sprites.sp[sp].ctl = value;
                denise->sprites.sp[sp].armed = true;
                if (sp & 1u) {
                    if (value & (1u << 7))
                        denise->sprites.attached_mask |= (rigel_u16)(1u << sp);
                    else
                        denise->sprites.attached_mask &= (rigel_u16)~(1u << sp);
                }
                break;
            case 4: /* DATA */
                denise->sprites.sp[sp].data = value;
                break;
            case 6: /* DATB */
                denise->sprites.sp[sp].datb = value;
                break;
            default:
                break;
            }
        }
        return;
    }

    switch (addr) {
    case RIGEL_REG_CLXCON:
        collision_write_clxcon(&denise->coll, value);
        break;
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
