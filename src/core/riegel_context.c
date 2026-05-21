#include "core/riegel_context.h"

riegel_u16 riegel_context_read_reg(const RiegelContext *ctx, riegel_u32 addr)
{
    riegel_u32 index;

    if (ctx == NULL || (addr & 1u) != 0 || addr >= RIEGEL_CUSTOM_SPACE_SIZE) {
        return 0;
    }

    index = addr >> 1;
    return ctx->custom_regs[index];
}

void riegel_context_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value)
{
    riegel_u32 index;

    if (ctx == NULL || (addr & 1u) != 0 || addr >= RIEGEL_CUSTOM_SPACE_SIZE) {
        return;
    }

    index = addr >> 1;
    ctx->custom_regs[index] = value;
}
