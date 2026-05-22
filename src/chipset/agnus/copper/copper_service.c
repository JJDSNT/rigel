#include "agnus/copper/copper_service.h"

#include "agnus/agnus_state.h"
#include "core/rigel_context.h"
#include "domains/copper/copper_domain.h"
#include "mmio/custom_regs.h"

static rigel_u16 rigel_agnus_copper_fetch16(RigelContext *ctx, rigel_u32 addr)
{
    addr &= 0x001ffffeu;

    if (ctx == NULL || ctx->config.chip_ram.read16 == NULL) {
        return 0;
    }

    return ctx->config.chip_ram.read16(ctx->config.chip_ram.opaque, addr);
}

void rigel_copper_service_step_program(RigelContext *ctx)
{
    RigelAgnus *agnus;
    copper_state_t *copper;
    rigel_u16 ir1;
    rigel_u16 ir2;
    rigel_u32 reg;

    if (ctx == NULL) {
        return;
    }

    agnus = &ctx->chipset.agnus;
    copper = &agnus->copper;
    if (!copper->enabled || !copper->fetch_pending) {
        return;
    }

    ir1 = rigel_agnus_copper_fetch16(ctx, copper->program_counter);
    ir2 = rigel_agnus_copper_fetch16(ctx, copper->program_counter + 2u);
    copper->fetch_pending = false;

    if ((ir2 & 0x0001u) != 0) {
        rigel_u16 wait_vpos = (rigel_u16)(ir1 >> 8);
        rigel_u16 wait_hpos = (rigel_u16)(ir1 & 0xFEu);
        rigel_u16 vpmask    = (rigel_u16)((ir2 >> 8) & 0xFFu);
        rigel_u16 hpmask    = (rigel_u16)(ir2 & 0xFEu);

        if (ir1 & 0x0001u) {
            /* SKIP: evaluate beam condition now; skip next instruction if satisfied */
            bool skip = copper_beam_cmp(agnus->beam.vpos, agnus->beam.hpos,
                                        wait_vpos, wait_hpos, vpmask, hpmask);
            copper->program_counter += skip ? 8u : 4u;
            copper->fetch_pending = true;
        } else {
            /* WAIT: arm beam-bound wait with masks */
            rigel_copper_domain_set_wait(copper, wait_vpos, wait_hpos, vpmask, hpmask);
        }
        return;
    }

    reg = (rigel_u32)(ir2 & 0x01feu);
    if (rigel_custom_is_valid_reg(reg)) {
        /* COPCON CDANG (bit 1): when clear, block writes to registers < 0x40 */
        bool cdang = (copper->copcon & 0x0002u) != 0;
        if (reg >= 0x40u || cdang) {
            custom_regs_write16(ctx, reg, ir1);
        }
    }

    copper->program_counter += 4u;
    copper->triggered = true;
    copper->fetch_pending = true;
}
