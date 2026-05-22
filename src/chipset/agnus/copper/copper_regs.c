#include "agnus/copper/copper_regs.h"

#include "agnus/copper/copper.h"
#include "core/rigel_context.h"
#include "domains/copper/copper_domain.h"
#include "rigel/rigel_custom.h"

bool rigel_copper_regs_owns_reg(rigel_u32 addr)
{
    switch (addr) {
    case RIGEL_REG_COP1LCH:
    case RIGEL_REG_COP1LCL:
    case RIGEL_REG_COP2LCH:
    case RIGEL_REG_COP2LCL:
    case RIGEL_REG_COPJMP1:
    case RIGEL_REG_COPJMP2:
        return true;
    default:
        return false;
    }
}

rigel_u16 rigel_copper_regs_read(RigelContext *ctx, rigel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    switch (addr) {
    case RIGEL_REG_COP1LCH:
        return (rigel_u16)(ctx->chipset.agnus.copper.cop1lc >> 16);
    case RIGEL_REG_COP1LCL:
        return (rigel_u16)(ctx->chipset.agnus.copper.cop1lc & 0xffffu);
    case RIGEL_REG_COP2LCH:
        return (rigel_u16)(ctx->chipset.agnus.copper.cop2lc >> 16);
    case RIGEL_REG_COP2LCL:
        return (rigel_u16)(ctx->chipset.agnus.copper.cop2lc & 0xffffu);
    default:
        return rigel_context_read_reg(ctx, addr);
    }
}

void rigel_copper_regs_write(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    switch (addr) {
    case RIGEL_REG_COP1LCH:
        copper_set_pointer_hi(&ctx->chipset.agnus.copper.cop1lc, value);
        break;
    case RIGEL_REG_COP1LCL:
        copper_set_pointer_lo(&ctx->chipset.agnus.copper.cop1lc, value);
        break;
    case RIGEL_REG_COP2LCH:
        copper_set_pointer_hi(&ctx->chipset.agnus.copper.cop2lc, value);
        break;
    case RIGEL_REG_COP2LCL:
        copper_set_pointer_lo(&ctx->chipset.agnus.copper.cop2lc, value);
        break;
    case RIGEL_REG_COPJMP1:
        rigel_copper_domain_jump1(&ctx->chipset.agnus.copper);
        break;
    case RIGEL_REG_COPJMP2:
        rigel_copper_domain_jump2(&ctx->chipset.agnus.copper);
        break;
    default:
        break;
    }

    rigel_context_write_reg(ctx, addr, value);
}
