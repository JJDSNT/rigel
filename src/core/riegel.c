#include "riegel/riegel.h"

#include <stdlib.h>
#include <string.h>

#include "chipset/chipset.h"
#include "core/riegel_context.h"

RiegelContext *riegel_create(const riegel_config_t *config)
{
    RiegelContext *ctx;

    if (config == NULL) {
        return NULL;
    }

    ctx = (RiegelContext *)calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->config = *config;
    riegel_reset(ctx);
    return ctx;
}

RiegelChipset *riegel_get_chipset(RiegelContext *ctx)
{
    if (ctx == NULL) {
        return NULL;
    }

    return &ctx->chipset;
}

const RiegelChipset *riegel_get_chipset_const(const RiegelContext *ctx)
{
    if (ctx == NULL) {
        return NULL;
    }

    return &ctx->chipset;
}

void riegel_destroy(RiegelContext *ctx)
{
    free(ctx);
}

void riegel_reset(RiegelContext *ctx)
{
    if (ctx == NULL) {
        return;
    }

    riegel_chipset_reset(&ctx->chipset);
}

void riegel_step(RiegelContext *ctx, riegel_u32 cycles)
{
    if (ctx == NULL) {
        return;
    }

    riegel_chipset_step(&ctx->chipset, cycles);
}

void riegel_take_snapshot(const RiegelContext *ctx, riegel_snapshot_t *snapshot)
{
    if (ctx == NULL) {
        return;
    }

    riegel_chipset_take_snapshot(&ctx->chipset, snapshot);
}

bool riegel_save_state(const RiegelContext *ctx, void *buffer, size_t buffer_size)
{
    riegel_snapshot_t snapshot;

    if (ctx == NULL || buffer == NULL || buffer_size < sizeof(snapshot)) {
        return false;
    }

    riegel_take_snapshot(ctx, &snapshot);
    (void)memcpy(buffer, &snapshot, sizeof(snapshot));
    return true;
}

bool riegel_load_state(RiegelContext *ctx, const void *buffer, size_t buffer_size)
{
    riegel_snapshot_t snapshot;

    if (ctx == NULL || buffer == NULL || buffer_size < sizeof(snapshot)) {
        return false;
    }

    (void)memcpy(&snapshot, buffer, sizeof(snapshot));
    ctx->chipset.cycles = snapshot.cycles;
    ctx->chipset.intreq = snapshot.intreq;
    ctx->chipset.intena = snapshot.intena;
    riegel_context_write_reg(ctx, 0x009a, ctx->chipset.intena);
    riegel_context_write_reg(ctx, 0x009c, ctx->chipset.intreq);
    return true;
}
