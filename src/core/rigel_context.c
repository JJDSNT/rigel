#include "core/rigel_context.h"

#include "rigel/rigel_custom.h"

rigel_u16 rigel_context_read_reg(const RigelContext *ctx, rigel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    return rigel_chipset_read_reg(&ctx->chipset, addr);
}

void rigel_context_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    rigel_chipset_write_reg(&ctx->chipset, addr, value);
}
