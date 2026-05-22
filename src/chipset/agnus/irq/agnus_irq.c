#include "agnus_irq.h"
#include "chipset/chipset.h"
#include "core/rigel_context.h"

/* TODO(irq): replace direct chipset call with an IRQ sink like the blitter uses */

void agnus_irq_raise_blitter_done(RigelContext *ctx)
{
    if (ctx) rigel_chipset_raise_irq_source(&ctx->chipset, AGNUS_INTB_BLIT);
}

void agnus_irq_raise_vblank(RigelContext *ctx)
{
    if (ctx) rigel_chipset_raise_irq_source(&ctx->chipset, AGNUS_INTB_VERTB);
}

void agnus_irq_raise_copper(RigelContext *ctx)
{
    if (ctx) rigel_chipset_raise_irq_source(&ctx->chipset, AGNUS_INTB_COPER);
}
