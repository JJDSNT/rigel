#include "agnus_irq.h"
#include "chipset/chipset.h"
#include "core/rigel_context.h"

void agnus_irq_raise_vblank(RigelContext *ctx)
{
    if (ctx) rigel_chipset_raise_irq_source(&ctx->chipset, AGNUS_INTB_VERTB);
}
