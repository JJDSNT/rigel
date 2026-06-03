#include "core/rigel_context.h"

#include "rigel/rigel_custom.h"

static rigel_u32 rigel_context_chip_ram_visible_size(const RigelContext *ctx)
{
    rigel_u32 model_limit;

    if (ctx == NULL) {
        return 0;
    }

    model_limit = ctx->config.chipset_model == RIGEL_CHIPSET_ECS
        ? 0x00100000u
        : 0x00080000u;

    if (ctx->config.chip_ram_size != 0u &&
        ctx->config.chip_ram_size < model_limit) {
        return ctx->config.chip_ram_size;
    }

    return model_limit;
}

static int rigel_context_chip_ram_map_addr(const RigelContext *ctx, rigel_u32 addr, rigel_u32 *mapped)
{
    rigel_u32 visible_size = rigel_context_chip_ram_visible_size(ctx);

    addr &= ~1u;
    if (visible_size != 0u && addr >= visible_size) {
        return 0;
    }

    if (mapped != NULL) {
        *mapped = addr;
    }
    return 1;
}

static rigel_u16 rigel_context_chip_ram_read16(void *opaque, rigel_u32 addr)
{
    RigelContext *ctx = (RigelContext *)opaque;
    rigel_u32 mapped;

    if (ctx == NULL || ctx->config.chip_ram.read16 == NULL) {
        return 0;
    }

    if (!rigel_context_chip_ram_map_addr(ctx, addr, &mapped)) {
        return 0xffffu;
    }

    return ctx->config.chip_ram.read16(
        ctx->config.chip_ram.opaque,
        mapped
    );
}

static void rigel_context_chip_ram_write16(void *opaque, rigel_u32 addr, rigel_u16 value)
{
    RigelContext *ctx = (RigelContext *)opaque;
    rigel_u32 mapped;

    if (ctx == NULL || ctx->config.chip_ram.write16 == NULL) {
        return;
    }

    if (!rigel_context_chip_ram_map_addr(ctx, addr, &mapped)) {
        return;
    }

    ctx->config.chip_ram.write16(
        ctx->config.chip_ram.opaque,
        mapped,
        value
    );
}

rigel_chip_ram_if_t rigel_context_chip_ram(RigelContext *ctx)
{
    rigel_chip_ram_if_t mem = { 0 };

    if (ctx == NULL) {
        return mem;
    }

    mem.opaque = ctx;
    mem.read16 = rigel_context_chip_ram_read16;
    mem.write16 = rigel_context_chip_ram_write16;
    return mem;
}

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
