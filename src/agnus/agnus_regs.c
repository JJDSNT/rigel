#include "agnus/agnus_regs.h"

#include "agnus/blitter/blitter.h"
#include "core/riegel_context.h"
#include "riegel/riegel_custom.h"

static riegel_u16 riegel_apply_setclr(riegel_u16 current, riegel_u16 value)
{
    riegel_u16 mask = (riegel_u16)(value & 0x7fffU);

    if ((value & RIEGEL_SETCLR) != 0) {
        return (riegel_u16)(current | mask);
    }

    return (riegel_u16)(current & (riegel_u16)(~mask));
}

static bool agnus_blitter_owns_reg(riegel_u32 addr)
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

bool agnus_owns_reg(riegel_u32 addr)
{
    if (addr == RIEGEL_REG_DMACON) {
        return true;
    }

    return agnus_blitter_owns_reg(addr);
}

riegel_u16 agnus_read_reg(RiegelContext *ctx, riegel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    if (agnus_blitter_owns_reg(addr)) {
        return blitter_read_reg16(&ctx->chipset.agnus.blitter, addr);
    }

    switch (addr) {
    case RIEGEL_REG_DMACON:
        return ctx->chipset.agnus.dmacon;
    default:
        return riegel_context_read_reg(ctx, addr);
    }
}

void agnus_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    if (agnus_blitter_owns_reg(addr)) {
        blitter_write_reg16(&ctx->chipset.agnus.blitter, addr, value);
        riegel_context_write_reg(ctx, addr, value);
        return;
    }

    switch (addr) {
    case RIEGEL_REG_DMACON:
        ctx->chipset.agnus.dmacon = riegel_apply_setclr(ctx->chipset.agnus.dmacon, value);
        riegel_context_write_reg(ctx, addr, ctx->chipset.agnus.dmacon);
        break;
    default:
        riegel_context_write_reg(ctx, addr, value);
        break;
    }
}
