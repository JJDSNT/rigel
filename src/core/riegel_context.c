#include "core/riegel_context.h"

#include "riegel/riegel_custom.h"

riegel_u16 riegel_context_read_reg(const RiegelContext *ctx, riegel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    return riegel_chipset_read_reg(&ctx->chipset, addr);
}

void riegel_context_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    riegel_chipset_write_reg(&ctx->chipset, addr, value);
}
