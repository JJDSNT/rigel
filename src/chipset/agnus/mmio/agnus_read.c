#include "agnus/mmio/agnus_mmio.h"

#include <stddef.h>

#include "core/rigel_context.h"
#include "domains/blitter/blitter_domain.h"
#include "domains/dma/dma_domain.h"
#include "rigel/rigel_custom.h"

rigel_u16 rigel_agnus_mmio_read_impl(RigelContext *ctx, rigel_u32 addr)
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
