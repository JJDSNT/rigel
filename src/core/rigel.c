#include "rigel/rigel.h"

#include <stdlib.h>
#include <string.h>

#include "chipset/chipset.h"
#include "core/rigel_context.h"
#include "floppy/floppy_drive.h"
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
    rigel_paula_set_serial_irq_sink(&ctx->chipset.paula, rigel_paula_serial_irq_sink(ctx));
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

void rigel_input_set_joydat(RigelContext *ctx, rigel_u32 port, rigel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    rigel_paula_set_joydat(&ctx->chipset.paula, port, value);
}

void rigel_input_set_pot_button_x(RigelContext *ctx, rigel_u32 port, bool pressed)
{
    if (ctx == NULL) {
        return;
    }

    rigel_paula_set_pot_button_x(&ctx->chipset.paula, port, pressed);
}

void rigel_input_set_pot_button_y(RigelContext *ctx, rigel_u32 port, bool pressed)
{
    if (ctx == NULL) {
        return;
    }

    rigel_paula_set_pot_button_y(&ctx->chipset.paula, port, pressed);
}

rigel_status_t rigel_floppy_insert(
    RigelContext *ctx,
    rigel_floppy_drive_id_t drive,
    const rigel_u8 *data,
    rigel_u32 size
)
{
    FloppyDrive *target;

    if (ctx == NULL || data == NULL || size == 0) {
        return RIGEL_STATUS_ERROR;
    }

    target = rigel_chipset_floppy_drive(&ctx->chipset, drive);
    if (target == NULL) {
        return RIGEL_STATUS_ERROR;
    }

    floppy_insert(target, data, size);
    return RIGEL_STATUS_OK;
}

void rigel_floppy_eject(RigelContext *ctx, rigel_floppy_drive_id_t drive)
{
    FloppyDrive *target;

    if (ctx == NULL) {
        return;
    }

    target = rigel_chipset_floppy_drive(&ctx->chipset, drive);
    if (target == NULL) {
        return;
    }

    floppy_eject(target);
}

bool rigel_floppy_has_media(const RigelContext *ctx, rigel_floppy_drive_id_t drive)
{
    const FloppyDrive *target;

    if (ctx == NULL) {
        return false;
    }

    target = rigel_chipset_floppy_drive_const(&ctx->chipset, drive);
    if (target == NULL) {
        return false;
    }

    return floppy_has_media(target) != 0;
}

bool rigel_floppy_get_status(
    const RigelContext *ctx,
    rigel_floppy_drive_id_t drive,
    rigel_floppy_status_t *status
)
{
    const FloppyDrive *target;

    if (ctx == NULL || status == NULL) {
        return false;
    }

    target = rigel_chipset_floppy_drive_const(&ctx->chipset, drive);
    if (target == NULL) {
        return false;
    }

    status->has_media = floppy_has_media(target) != 0;
    status->motor_on = target->motor != 0;
    status->ready = target->ready != 0;
    status->track0 = target->track0 != 0;
    status->disk_changed = target->disk_changed != 0;
    status->write_protected = target->write_protected != 0;
    status->dma_active =
        drive == RIGEL_FLOPPY_DRIVE_DF0 &&
        ctx->chipset.paula.disk.drive == target &&
        ctx->chipset.paula.disk.dma_active != 0;
    status->cylinder = (rigel_u8)target->cylinder;
    status->side = (rigel_u8)target->side;
    return true;
}

rigel_rtc_model_t rigel_rtc_get_model(const RigelContext *ctx)
{
    if (ctx == NULL) {
        return RIGEL_RTC_MODEL_NONE;
    }

    return rtc_get_model(&ctx->chipset.rtc);
}

void rigel_rtc_set_model(RigelContext *ctx, rigel_rtc_model_t model)
{
    if (ctx == NULL) {
        return;
    }

    rtc_set_model(&ctx->chipset.rtc, model);
}

time_t rigel_rtc_get_time(RigelContext *ctx)
{
    if (ctx == NULL) {
        return (time_t)0;
    }

    return rtc_get_time(&ctx->chipset.rtc);
}

void rigel_rtc_set_time(RigelContext *ctx, time_t value)
{
    if (ctx == NULL) {
        return;
    }

    rtc_set_time(&ctx->chipset.rtc, value);
    rtc_update(&ctx->chipset.rtc);
}

rigel_u8 rigel_rtc_read_reg(RigelContext *ctx, rigel_u8 reg)
{
    if (ctx == NULL || reg >= 16u) {
        return 0;
    }

    return rtc_read_reg(&ctx->chipset.rtc, reg);
}

void rigel_rtc_write_reg(RigelContext *ctx, rigel_u8 reg, rigel_u8 value)
{
    if (ctx == NULL || reg >= 16u) {
        return;
    }

    rtc_write_reg(&ctx->chipset.rtc, reg, value);
}
