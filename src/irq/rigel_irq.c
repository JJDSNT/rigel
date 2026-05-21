#include "rigel/rigel_irq.h"

#include "core/rigel_context.h"
#include "paula/paula_interrupts.h"

rigel_u16 rigel_get_intreq(const RigelContext *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    return rigel_paula_interrupts_read_intreq(&ctx->chipset.paula.interrupts);
}

rigel_u16 rigel_get_intena(const RigelContext *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    return rigel_paula_interrupts_read_intena(&ctx->chipset.paula.interrupts);
}

rigel_u8 rigel_get_ipl(const RigelContext *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    return rigel_paula_interrupts_current_ipl(&ctx->chipset.paula.interrupts);
}
