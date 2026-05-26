#include "agnus/mmio/agnus_mmio.h"

#include <stddef.h>

#include "agnus/bitplanes/bitplane_pointers.h"
#include "agnus/copper/copper_regs.h"
#include "agnus/dma/sprite_dma.h"
#include "agnus/mmio/agnus_regs.h"

#include "agnus/timing/raster.h"
#include "agnus/timing/slot_scheduler.h"
#include "core/rigel_context.h"
#include "domains/blitter/blitter_domain.h"
#include "domains/dma/dma_domain.h"
#include "rigel/rigel_custom.h"

void rigel_agnus_mmio_write_impl(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    if (rigel_blitter_domain_owns_reg(addr)) {
        rigel_blitter_domain_write_reg(&ctx->chipset.agnus.blitter, addr, value);
        rigel_context_write_reg(ctx, addr, value);
        return;
    }

    if (rigel_copper_regs_owns_reg(addr)) {
        rigel_copper_regs_write(ctx, addr, value);
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
        agnus_slot_scheduler_invalidate(
            &ctx->chipset.agnus.scheduler,
            rigel_dma_domain_read_dmacon(&ctx->chipset.agnus.dma)
        );
        break;
    case RIGEL_REG_DDFSTRT:
        raster_set_ddfstrt(&ctx->chipset.agnus.raster, value);
        agnus_slot_scheduler_set_ddf(
            &ctx->chipset.agnus.scheduler,
            value,
            ctx->chipset.agnus.scheduler.ddfstop
        );
        rigel_context_write_reg(ctx, addr, value);
        break;
    case RIGEL_REG_DDFSTOP:
        raster_set_ddfstop(&ctx->chipset.agnus.raster, value);
        agnus_slot_scheduler_set_ddf(
            &ctx->chipset.agnus.scheduler,
            ctx->chipset.agnus.scheduler.ddfstrt,
            value
        );
        rigel_context_write_reg(ctx, addr, value);
        break;
    case AGNUS_BPLMOD1:
        bplpt_set_modulo(&ctx->chipset.agnus.bplpt, 0, value);
        rigel_context_write_reg(ctx, addr, value);
        break;
    case AGNUS_BPLMOD2:
        bplpt_set_modulo(&ctx->chipset.agnus.bplpt, 1, value);
        rigel_context_write_reg(ctx, addr, value);
        break;
    default:
        /* BPL1PTH-BPL6PTL */
        if (addr >= RIGEL_REG_BPL1PTH && addr <= RIGEL_REG_BPL6PTL && (addr & 1u) == 0u) {
            unsigned plane = (unsigned)((addr - RIGEL_REG_BPL1PTH) / 4u);
            if ((addr & 2u) == 0u)
                bplpt_set_hi(&ctx->chipset.agnus.bplpt, plane, value);
            else
                bplpt_set_lo(&ctx->chipset.agnus.bplpt, plane, value);
        }
        /* SPR0PTH-SPR7PTL */
        if (addr >= AGNUS_SPR0PTH && addr <= AGNUS_SPR7PTL && (addr & 1u) == 0u) {
            unsigned sp = (unsigned)((addr - AGNUS_SPR0PTH) / 4u);
            if ((addr & 2u) == 0u)
                sprite_dma_set_ptr_hi(&ctx->chipset.agnus.sprite_dma, sp, value);
            else
                sprite_dma_set_ptr_lo(&ctx->chipset.agnus.sprite_dma, sp, value);
        }
        rigel_context_write_reg(ctx, addr, value);
        break;
    }
}
