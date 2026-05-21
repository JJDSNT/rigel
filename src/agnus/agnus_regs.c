#include "agnus/agnus_regs.h"

#include "agnus/blitter/blitter.h"
#include "core/rigel_context.h"
#include "rigel/rigel_custom.h"

static rigel_u16 rigel_apply_setclr(rigel_u16 current, rigel_u16 value)
{
    rigel_u16 mask = (rigel_u16)(value & 0x7fffU);

    if ((value & RIGEL_SETCLR) != 0) {
        return (rigel_u16)(current | mask);
    }

    return (rigel_u16)(current & (rigel_u16)(~mask));
}

static bool agnus_blitter_owns_reg(rigel_u32 addr)
{
    switch (addr) {
    case 0x000:
    case 0x002:
    case 0x040:
    case 0x042:
    case 0x044:
    case 0x046:
    case 0x048:
    case 0x04a:
    case 0x04c:
    case 0x04e:
    case 0x050:
    case 0x052:
    case 0x054:
    case 0x056:
    case 0x058:
    case 0x060:
    case 0x062:
    case 0x064:
    case 0x066:
    case 0x070:
    case 0x072:
    case 0x074:
        return true;
    default:
        return false;
    }
}

bool agnus_owns_reg(rigel_u32 addr)
{
    if (addr == RIGEL_REG_DMACON) {
        return true;
    }

    return agnus_blitter_owns_reg(addr);
}

rigel_u16 agnus_read_reg(RigelContext *ctx, rigel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    if (agnus_blitter_owns_reg(addr)) {
        return blitter_read_reg16(&ctx->chipset.agnus.blitter, addr);
    }

    switch (addr) {
    case RIGEL_REG_DMACON:
        return ctx->chipset.agnus.dmacon;
    default:
        return rigel_context_read_reg(ctx, addr);
    }
}

void agnus_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    if (agnus_blitter_owns_reg(addr)) {
        blitter_write_reg16(&ctx->chipset.agnus.blitter, addr, value);
        rigel_context_write_reg(ctx, addr, value);
        return;
    }

    switch (addr) {
    case RIGEL_REG_DMACON:
        ctx->chipset.agnus.dmacon = rigel_apply_setclr(ctx->chipset.agnus.dmacon, value);
        rigel_context_write_reg(ctx, addr, ctx->chipset.agnus.dmacon);
        break;
    default:
        rigel_context_write_reg(ctx, addr, value);
        break;
    }
}
