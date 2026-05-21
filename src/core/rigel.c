#include "rigel/rigel.h"

#include <stdlib.h>
#include <string.h>

#include "chipset/chipset.h"
#include "core/rigel_context.h"
#include "paula/paula_interrupts.h"
#include "paula/paula_state.h"

RigelContext *rigel_create(const rigel_config_t *config)
{
    RigelContext *ctx;

    if (config == NULL) {
        return NULL;
    }

    ctx = (RigelContext *)calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->config = *config;
    rigel_reset(ctx);
    rigel_paula_set_disk_memory_if(&ctx->chipset.paula, ctx->config.chip_ram);
    rigel_paula_set_disk_irq_sink(&ctx->chipset.paula, rigel_paula_disk_irq_sink(ctx));
    return ctx;
}

RigelChipset *rigel_get_chipset(RigelContext *ctx)
{
    if (ctx == NULL) {
        return NULL;
    }

    return &ctx->chipset;
}

const RigelChipset *rigel_get_chipset_const(const RigelContext *ctx)
{
    if (ctx == NULL) {
        return NULL;
    }

    return &ctx->chipset;
}

void rigel_destroy(RigelContext *ctx)
{
    free(ctx);
}

void rigel_reset(RigelContext *ctx)
{
    if (ctx == NULL) {
        return;
    }

    rigel_chipset_reset(&ctx->chipset);
}

void rigel_step(RigelContext *ctx, rigel_u32 cycles)
{
    if (ctx == NULL) {
        return;
    }

    rigel_chipset_step(ctx, cycles);
}

void rigel_take_snapshot(const RigelContext *ctx, rigel_snapshot_t *snapshot)
{
    if (ctx == NULL) {
        return;
    }

    rigel_chipset_take_snapshot(&ctx->chipset, snapshot);
}

bool rigel_save_state(const RigelContext *ctx, void *buffer, size_t buffer_size)
{
    rigel_snapshot_t snapshot;

    if (ctx == NULL || buffer == NULL || buffer_size < sizeof(snapshot)) {
        return false;
    }

    rigel_take_snapshot(ctx, &snapshot);
    (void)memcpy(buffer, &snapshot, sizeof(snapshot));
    return true;
}

bool rigel_load_state(RigelContext *ctx, const void *buffer, size_t buffer_size)
{
    rigel_snapshot_t snapshot;

    if (ctx == NULL || buffer == NULL || buffer_size < sizeof(snapshot)) {
        return false;
    }

    (void)memcpy(&snapshot, buffer, sizeof(snapshot));
    ctx->chipset.cycles = snapshot.cycles;
    rigel_paula_interrupts_write_intena(&ctx->chipset.paula.interrupts, RIGEL_PAULA_INT_SETCLR | snapshot.intena);
    rigel_paula_interrupts_write_intreq(&ctx->chipset.paula.interrupts, RIGEL_PAULA_INT_SETCLR | snapshot.intreq);
    rigel_context_write_reg(ctx, 0x009a, rigel_paula_interrupts_read_intena(&ctx->chipset.paula.interrupts));
    rigel_context_write_reg(ctx, 0x009c, rigel_paula_interrupts_read_intreq(&ctx->chipset.paula.interrupts));
    return true;
}
