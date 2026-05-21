#include "riegel/riegel_irq.h"

#include "core/riegel_context.h"

riegel_u16 riegel_get_intreq(const RiegelContext *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    return ctx->intreq;
}

riegel_u16 riegel_get_intena(const RiegelContext *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    return ctx->intena;
}

riegel_u8 riegel_get_ipl(const RiegelContext *ctx)
{
    riegel_u16 pending;

    if (ctx == NULL) {
        return 0;
    }

    pending = (riegel_u16)(ctx->intreq & ctx->intena);
    return pending != 0 ? 1u : 0u;
}
