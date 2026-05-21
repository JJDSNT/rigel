#include "agnus/agnus_regs.h"

#include "core/rigel_context.h"
#include "domains/blitter/blitter_domain.h"
#include "domains/dma/dma_domain.h"
#include "rigel/rigel_custom.h"

bool agnus_owns_reg(rigel_u32 addr)
{
    if (addr == RIGEL_REG_DMACON) {
        return true;
    }

    return rigel_blitter_domain_owns_reg(addr);
}

rigel_u16 agnus_read_reg(RigelContext *ctx, rigel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    if (rigel_blitter_domain_owns_reg(addr)) {
        return rigel_blitter_domain_read_reg(&ctx->chipset.agnus.blitter, addr);
    }

    switch (addr) {
    case RIGEL_REG_DMACON:
        return rigel_dma_domain_read_dmacon(&ctx->chipset.agnus.dma);
    default:
        return rigel_context_read_reg(ctx, addr);
    }
}

void agnus_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    if (rigel_blitter_domain_owns_reg(addr)) {
        rigel_blitter_domain_write_reg(&ctx->chipset.agnus.blitter, addr, value);
        rigel_context_write_reg(ctx, addr, value);
        return;
    }

    switch (addr) {
    case RIGEL_REG_DMACON:
        rigel_dma_domain_write_dmacon(&ctx->chipset.agnus.dma, value);
        rigel_context_write_reg(
            ctx,
            addr,
            rigel_dma_domain_read_dmacon(&ctx->chipset.agnus.dma)
        );
        break;
    default:
        rigel_context_write_reg(ctx, addr, value);
        break;
    }
}
