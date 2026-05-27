#include "agnus/mmio/agnus_mmio.h"

#include <stddef.h>

#include "agnus/blitter/blitter.h"
#include "agnus/copper/copper_regs.h"
#include "agnus/mmio/agnus_regs.h"
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

    if (rigel_copper_regs_owns_reg(addr)) {
        return rigel_copper_regs_read(ctx, addr);
    }

    switch (addr) {
    case AGNUS_DMACONR: {
        /* Live DMACON plus blitter-status bits BBUSY (14) and BZERO (13). */
        rigel_u16 val = rigel_dma_domain_read_dmacon(&ctx->chipset.agnus.dma);
        if (blitter_is_busy(&ctx->chipset.agnus.blitter))
            val |= (rigel_u16)AGNUS_DMACON_BBUSY;
        if (!ctx->chipset.agnus.blitter.result.zero)
            val |= (rigel_u16)AGNUS_DMACON_BZERO;
        return val;
    }
    case RIGEL_REG_DMACON:
        return rigel_dma_domain_read_dmacon(&ctx->chipset.agnus.dma);
    case AGNUS_VPOSR:
        /* [15]=LOF [14:8]=chip_id (0x00 = OCS Agnus 8361/8370) [0]=vpos[8] */
        return (rigel_u16)(
            ((rigel_u16)ctx->chipset.agnus.beam.lof << 15) |
            ((ctx->chipset.agnus.beam.vpos >> 8) & 1u)
        );
    case AGNUS_VHPOSR:
        /* [15:8]=vpos[7:0]  [7:0]=hpos in lores pixels (CCK / 2) */
        return (rigel_u16)(((ctx->chipset.agnus.beam.vpos & 0xFFu) << 8) |
                            ((ctx->chipset.agnus.beam.hpos >> 1) & 0xFFu));
    default:
        return rigel_context_read_reg(ctx, addr);
    }
}
