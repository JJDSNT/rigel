#include "core/riegel_context.h"

#include "riegel/riegel_custom.h"

riegel_u16 riegel_context_read_reg(const RiegelContext *ctx, riegel_u32 addr)
{
    riegel_u32 index;

    if (ctx == NULL || !riegel_custom_is_valid_reg(addr)) {
        return 0;
    }

    index = addr >> 1;
    return ctx->custom_regs[index];
}

void riegel_context_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value)
{
    riegel_u32 index;

    if (ctx == NULL || !riegel_custom_is_valid_reg(addr)) {
        return;
    }

    index = addr >> 1;
    ctx->custom_regs[index] = value;
}
