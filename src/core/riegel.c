#include "riegel/riegel.h"

#include <stdlib.h>
#include <string.h>

#include "chipset/chipset.h"
#include "core/riegel_context.h"
#include "paula/paula_interrupts.h"
#include "paula/paula_state.h"

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
    riegel_paula_set_disk_memory_if(&ctx->chipset.paula, ctx->config.chip_ram);
    riegel_paula_set_disk_irq_sink(&ctx->chipset.paula, riegel_paula_disk_irq_sink(ctx));
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
    riegel_paula_interrupts_write_intena(&ctx->chipset.paula.interrupts, RIEGEL_PAULA_INT_SETCLR | snapshot.intena);
    riegel_paula_interrupts_write_intreq(&ctx->chipset.paula.interrupts, RIEGEL_PAULA_INT_SETCLR | snapshot.intreq);
    riegel_context_write_reg(ctx, 0x009a, riegel_paula_interrupts_read_intena(&ctx->chipset.paula.interrupts));
    riegel_context_write_reg(ctx, 0x009c, riegel_paula_interrupts_read_intreq(&ctx->chipset.paula.interrupts));
    return true;
}
