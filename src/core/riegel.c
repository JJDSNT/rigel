#include "riegel/riegel.h"

#include <stdlib.h>
#include <string.h>

#include "core/riegel_context.h"
#include "core/riegel_timing.h"

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

void riegel_destroy(RiegelContext *ctx)
{
    free(ctx);
}

void riegel_reset(RiegelContext *ctx)
{
    if (ctx == NULL) {
        return;
    }

    ctx->cycles = 0;
    ctx->intreq = 0;
    ctx->intena = 0;
    (void)memset(ctx->custom_regs, 0, sizeof(ctx->custom_regs));
}

void riegel_step(RiegelContext *ctx, riegel_u32 cycles)
{
    if (ctx == NULL) {
        return;
    }

    ctx->cycles = riegel_timing_advance(ctx->cycles, cycles);
}

void riegel_take_snapshot(const RiegelContext *ctx, riegel_snapshot_t *snapshot)
{
    if (ctx == NULL || snapshot == NULL) {
        return;
    }

    snapshot->cycles = ctx->cycles;
    snapshot->intreq = ctx->intreq;
    snapshot->intena = ctx->intena;
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
    ctx->cycles = snapshot.cycles;
    ctx->intreq = snapshot.intreq;
    ctx->intena = snapshot.intena;
    riegel_context_write_reg(ctx, 0x009a, ctx->intena);
    riegel_context_write_reg(ctx, 0x009c, ctx->intreq);
    return true;
}
