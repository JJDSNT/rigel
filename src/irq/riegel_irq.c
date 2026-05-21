#include "riegel/riegel_irq.h"

#include "core/riegel_context.h"
#include "paula/paula_interrupts.h"

riegel_u16 riegel_get_intreq(const RiegelContext *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    return riegel_paula_interrupts_read_intreq(&ctx->chipset.paula.interrupts);
}

riegel_u16 riegel_get_intena(const RiegelContext *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    return riegel_paula_interrupts_read_intena(&ctx->chipset.paula.interrupts);
}

riegel_u8 riegel_get_ipl(const RiegelContext *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    return riegel_paula_interrupts_current_ipl(&ctx->chipset.paula.interrupts);
}
