#include "core/rigel_context.h"

#include "rigel/rigel_custom.h"

#if RIGEL_ENABLE_STDLIB_ENV
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#endif

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
    /*
     * Chip RAM is mirrored: addresses beyond the physical size wrap modulo
     * visible_size (hardware behaviour).  Reject-on-overflow was wrong and
     * broke the chip-RAM masking contract tested by test_mmio.
     */
    if (visible_size > 0u) {
        addr &= visible_size - 1u;
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
#if RIGEL_ENABLE_STDLIB_ENV
        {
            static int warned = 0;
            const char *e = getenv("RIGEL_BPL_FETCH_PROBE");
            if (!warned && e && e[0] != '\0' && e[0] != '0') {
                warned = 1;
                printf("[BPL-CTX-PROBE] EARLY-NULL ctx=%p fn=%p addr=%06x\n",
                       (void *)ctx,
                       ctx ? (void *)(uintptr_t)ctx->config.chip_ram.read16 : NULL,
                       (unsigned)addr);
            }
        }
#endif
        return 0;
    }

    if (!rigel_context_chip_ram_map_addr(ctx, addr, &mapped)) {
        return 0xffffu;
    }

#if RIGEL_ENABLE_STDLIB_ENV
    {
        static int p_enabled = -1;
        static unsigned p_count = 0u;
        if (p_enabled < 0) {
            const char *e = getenv("RIGEL_BPL_FETCH_PROBE");
            p_enabled = (e != NULL && e[0] != '\0' && e[0] != '0') ? 1 : 0;
        }
        if (p_enabled && p_count < 16u && addr >= 0xC0000u && addr < 0xD0000u) {
            rigel_u16 r = ctx->config.chip_ram.read16(ctx->config.chip_ram.opaque, mapped);
            printf("[BPL-CTX-PROBE] addr=%06x mapped=%06x inner_op=%p inner_fn=%p result=%04x null=%d\n",
                   (unsigned)addr, (unsigned)mapped,
                   ctx->config.chip_ram.opaque,
                   (void *)(uintptr_t)(ctx->config.chip_ram.read16),
                   (unsigned)r,
                   ctx->config.chip_ram.read16 == NULL ? 1 : 0);
            p_count++;
            return r;
        }
    }
#endif

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
