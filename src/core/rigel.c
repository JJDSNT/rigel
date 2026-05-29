#include "rigel/rigel.h"

#include <stdlib.h>
#include <string.h>

#include "chipset/chipset.h"
#include "cia/cia.h"
#include "debug/log.h"
#include "chipset/agnus/agnus_config.h"
#include "chipset/agnus/beam.h"
#include "chipset/agnus/blitter/blitter.h"
#include "chipset/agnus/timing/deadline.h"
#include "chipset/agnus/timing/slot_scheduler.h"
#include "chipset/agnus/timing/vblank.h"
#include "domains/copper/copper_domain.h"
#include "core/rigel_context.h"
#include "floppy/floppy_drive.h"
#include "paula/audio.h"
#include "paula/disk.h"
#include "paula/paula_interrupts.h"
#include "paula/paula_state.h"

enum { RIGEL_DMACON_BLTPRI = 0x0400u };

static rigel_bus_owner_t slot_to_bus_owner(agnus_slot_owner_t slot)
{
    switch (slot) {
    case AGNUS_SLOT_REFRESH:                              return RIGEL_BUS_OWNER_REFRESH;
    case AGNUS_SLOT_DISK:                                 return RIGEL_BUS_OWNER_DISK;
    case AGNUS_SLOT_AUDIO_0: case AGNUS_SLOT_AUDIO_1:
    case AGNUS_SLOT_AUDIO_2: case AGNUS_SLOT_AUDIO_3:    return RIGEL_BUS_OWNER_AUDIO;
    case AGNUS_SLOT_SPRITE_0: case AGNUS_SLOT_SPRITE_1:
    case AGNUS_SLOT_SPRITE_2: case AGNUS_SLOT_SPRITE_3:
    case AGNUS_SLOT_SPRITE_4: case AGNUS_SLOT_SPRITE_5:
    case AGNUS_SLOT_SPRITE_6: case AGNUS_SLOT_SPRITE_7:  return RIGEL_BUS_OWNER_SPRITE;
    case AGNUS_SLOT_BITPLANE:                             return RIGEL_BUS_OWNER_BITPLANE;
    case AGNUS_SLOT_COPPER:                               return RIGEL_BUS_OWNER_COPPER;
    case AGNUS_SLOT_BLITTER:                              return RIGEL_BUS_OWNER_BLITTER;
    case AGNUS_SLOT_FREE: case AGNUS_SLOT_CPU: default:   return RIGEL_BUS_OWNER_CPU;
    }
}
enum { RIGEL_DEFAULT_CLOCK_HZ = 7093790u };

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

    if (config->log_fn != NULL) {
        rigel_log_set_fn(config->log_fn, config->log_opaque);
    }
    if (config->log_event_fn != NULL) {
        rigel_log_set_event_fn(config->log_event_fn, config->log_event_opaque);
    }

    ctx->config = *config;
    rigel_reset(ctx);

    rigel_paula_set_disk_memory_if(&ctx->chipset.paula, rigel_context_chip_ram(ctx));
    rigel_paula_set_disk_irq_sink(&ctx->chipset.paula, rigel_paula_disk_irq_sink(ctx));
    rigel_paula_set_serial_irq_sink(&ctx->chipset.paula, rigel_paula_serial_irq_sink(ctx));

    if (config->rtc_model != RIGEL_RTC_MODEL_NONE) {
        rtc_set_model(&ctx->chipset.rtc, config->rtc_model);
        if (config->rtc_time != 0) {
            rtc_set_time(&ctx->chipset.rtc, config->rtc_time);
        }
    }

    return ctx;
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
    rigel_agnus_set_chip_rev(
        &ctx->chipset.agnus,
        ctx->config.chipset_model == RIGEL_CHIPSET_ECS ? AGNUS_REV_ECS : AGNUS_REV_OCS
    );
    rigel_denise_set_chip_rev(
        &ctx->chipset.denise,
        ctx->config.chipset_model == RIGEL_CHIPSET_ECS ? AGNUS_REV_ECS : AGNUS_REV_OCS
    );
    rigel_agnus_set_video_std(
        &ctx->chipset.agnus,
        ctx->config.video_std == RIGEL_VIDEO_PAL ? AGNUS_VIDEO_PAL : AGNUS_VIDEO_NTSC
    );
    rigel_denise_set_framebuffer_target(&ctx->chipset.denise, &ctx->config.framebuffer);
    rigel_paula_set_disk_memory_if(&ctx->chipset.paula, rigel_context_chip_ram(ctx));
}

rigel_u32 rigel_get_clock_hz(const RigelContext *ctx)
{
    if (ctx == NULL || ctx->config.clock_hz == 0) {
        return RIGEL_DEFAULT_CLOCK_HZ;
    }

    return ctx->config.clock_hz;
}

rigel_u32 rigel_get_line_cycles(const RigelContext *ctx)
{
    if (ctx == NULL || ctx->chipset.agnus.beam.line_clocks == 0) {
        return RIGEL_BEAM_DEFAULT_LINE_CLOCKS;
    }

    return ctx->chipset.agnus.beam.line_clocks;
}

rigel_u32 rigel_get_frame_cycles(const RigelContext *ctx)
{
    rigel_u32 line_cycles;
    rigel_u32 frame_lines;

    line_cycles = rigel_get_line_cycles(ctx);
    if (ctx == NULL || ctx->chipset.agnus.beam.frame_lines == 0) {
        frame_lines = RIGEL_BEAM_DEFAULT_FRAME_LINES;
    } else {
        frame_lines = ctx->chipset.agnus.beam.frame_lines;
    }

    return line_cycles * frame_lines;
}

rigel_u64 rigel_cycles_to_us(rigel_cycle_t cycles, rigel_u32 clock_hz)
{
    if (clock_hz == 0) {
        clock_hz = RIGEL_DEFAULT_CLOCK_HZ;
    }

    return (rigel_u64)((cycles * 1000000u) / clock_hz);
}

rigel_cycle_t rigel_us_to_cycles(rigel_u64 microseconds, rigel_u32 clock_hz)
{
    if (clock_hz == 0) {
        clock_hz = RIGEL_DEFAULT_CLOCK_HZ;
    }

    return (rigel_cycle_t)((microseconds * clock_hz) / 1000000u);
}

rigel_cycle_t rigel_get_time(const RigelContext *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    return ctx->chipset.cycles;
}

rigel_cycle_t rigel_get_next_deadline(const RigelContext *ctx)
{
    agnus_deadlines_t d;
    rigel_u16 line_end;

    if (ctx == NULL) {
        return 0;
    }

    agnus_deadlines_reset(&d);

    if (blitter_is_busy(&ctx->chipset.agnus.blitter)) {
        d.blitter = ctx->chipset.agnus.blitter.cycles_remaining;
    }

    line_end = beam_cycles_until_line_end(&ctx->chipset.agnus.beam);
    if (line_end == 0) {
        line_end = RIGEL_BEAM_DEFAULT_LINE_CLOCKS;
    }
    d.beam_line_end = line_end;

    d.vertb = agnus_cycles_to_vertb(&ctx->chipset.agnus.beam);

    d.copper_wait = rigel_copper_domain_cycles_to_wait(
        &ctx->chipset.agnus.copper,
        &ctx->chipset.agnus.beam
    );

    d.audio = audio_cycles_to_next_event(&ctx->chipset.paula.audio);
    d.disk  = disk_cycles_to_next_event(&ctx->chipset.paula.disk);
    d.slot  = agnus_slot_scheduler_next_event(
        &ctx->chipset.agnus.scheduler,
        ctx->chipset.agnus.beam.line_clocks
    );

    return ctx->chipset.cycles + agnus_deadlines_min(&d);
}

rigel_step_result_t rigel_step(RigelContext *ctx, rigel_cycle_t cycles)
{
    rigel_step_result_t result;
    rigel_u8 ipl_before;
    bool blit_busy_before;
    rigel_u16 vpos_before;
    rigel_u64 frame_before;
    bool vblank_before;

    result.time = 0;
    result.events = RIGEL_EVENT_NONE;

    if (ctx == NULL) {
        return result;
    }

    ipl_before = rigel_paula_interrupts_current_ipl(&ctx->chipset.paula.interrupts);
    blit_busy_before = blitter_is_busy(&ctx->chipset.agnus.blitter) != 0;
    vpos_before = ctx->chipset.agnus.beam.vpos;
    ctx->chipset.agnus.copper.event_latched = false;
    frame_before = ctx->chipset.agnus.beam.frame_count;
    vblank_before = beam_in_vblank(&ctx->chipset.agnus.beam);

    rigel_chipset_step(ctx, (rigel_u32)cycles);

    result.time = ctx->chipset.cycles;

    if (rigel_paula_interrupts_current_ipl(&ctx->chipset.paula.interrupts) != ipl_before) {
        result.events |= (rigel_u32)RIGEL_EVENT_IRQ_CHANGED;
    }

    if (blit_busy_before && blitter_is_busy(&ctx->chipset.agnus.blitter) == 0) {
        result.events |= (rigel_u32)RIGEL_EVENT_BLIT_DONE;
    }

    if (ctx->chipset.agnus.copper.event_latched) {
        result.events |= (rigel_u32)RIGEL_EVENT_COPPER;
    }

    if (ctx->chipset.agnus.beam.frame_count != frame_before) {
        result.events |= (rigel_u32)RIGEL_EVENT_FRAME_READY;
    }

    if (ctx->chipset.agnus.beam.vpos != vpos_before || ctx->chipset.agnus.beam.frame_count != frame_before) {
        result.events |= (rigel_u32)RIGEL_EVENT_HBLANK;
    }

    if (!vblank_before && beam_in_vblank(&ctx->chipset.agnus.beam)) {
        result.events |= (rigel_u32)RIGEL_EVENT_VBLANK;
    }

    if (ctx->chipset.paula.audio.sample_ready) {
        result.events |= (rigel_u32)RIGEL_EVENT_AUDIO_READY;
        ctx->chipset.paula.audio.sample_ready = false;
    }

    return result;
}

rigel_step_result_t rigel_step_until(RigelContext *ctx, rigel_cycle_t target_time)
{
    rigel_step_result_t result;
    rigel_cycle_t current;

    result.time = 0;
    result.events = RIGEL_EVENT_NONE;

    if (ctx == NULL) {
        return result;
    }

    current = ctx->chipset.cycles;
    if (target_time <= current) {
        result.time = current;
        return result;
    }

    return rigel_step(ctx, target_time - current);
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

void rigel_input_set_fire(RigelContext *ctx, rigel_u32 port, bool pressed)
{
    rigel_u8 pra;

    if (ctx == NULL || port > 1u) {
        return;
    }

    /*
     * CIA-A PRA: bit 6 = /FIR0 (port 0), bit 7 = /FIR1 (port 1). Active low.
     * Read current external PRA, update the relevant bit, write back.
     */
    pra = cia_port_a_value(&ctx->chipset.cia[0]);
    if (port == 0u) {
        pra = pressed ? (pra & ~0x40u) : (pra | 0x40u);
    } else {
        pra = pressed ? (pra & ~0x80u) : (pra | 0x80u);
    }
    cia_set_external_pra(&ctx->chipset.cia[0], pra);
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

/* -------------------------------------------------------------------------
 * Bus observation
 * TODO: refine as domains gain slot-level scheduling
 * ------------------------------------------------------------------------- */

rigel_bus_state_t rigel_get_bus_state(const RigelContext *ctx)
{
    rigel_bus_state_t state;
    bool blit_busy;
    bool blt_pri;

    state.time = 0;
    state.owner = RIGEL_BUS_OWNER_NONE;
    state.active_dma = (rigel_u32)RIGEL_BUS_DMA_NONE;
    state.cpu_can_access_chip_ram = true;
    state.cpu_can_access_custom = true;
    state.cpu_would_stall = false;
    state.next_change = 0;

    if (ctx == NULL) {
        return state;
    }

    state.time = ctx->chipset.cycles;

    blit_busy = blitter_is_busy(&ctx->chipset.agnus.blitter) != 0;
    blt_pri = (ctx->chipset.agnus.dma.dmacon & RIGEL_DMACON_BLTPRI) != 0;

    if (blit_busy) {
        state.owner = RIGEL_BUS_OWNER_BLITTER;
        state.active_dma |= (rigel_u32)RIGEL_BUS_DMA_BLITTER;
        state.cpu_would_stall = blt_pri;
        state.next_change = ctx->chipset.cycles + ctx->chipset.agnus.blitter.cycles_remaining;
    } else {
        agnus_slot_owner_t slot_owner =
            agnus_slot_scheduler_current_owner(&ctx->chipset.agnus.scheduler);
        state.cpu_would_stall = agnus_slot_scheduler_cpu_stall(&ctx->chipset.agnus.scheduler);
        state.owner = slot_to_bus_owner(slot_owner);
        state.next_change = ctx->chipset.cycles +
            agnus_slot_scheduler_next_event(&ctx->chipset.agnus.scheduler,
                                            ctx->chipset.agnus.beam.line_clocks);
    }

    state.cpu_can_access_chip_ram = !state.cpu_would_stall;

    return state;
}

rigel_cycle_t rigel_get_next_bus_change(const RigelContext *ctx)
{
    return rigel_get_bus_state(ctx).next_change;
}

bool rigel_cpu_can_access_chip_ram(const RigelContext *ctx)
{
    return rigel_get_bus_state(ctx).cpu_can_access_chip_ram;
}

bool rigel_cpu_can_access_custom(const RigelContext *ctx)
{
    if (ctx == NULL) {
        return false;
    }

    return true;
}

rigel_cycle_t rigel_get_cpu_resume_time(const RigelContext *ctx)
{
    rigel_bus_state_t state = rigel_get_bus_state(ctx);

    if (!state.cpu_would_stall) {
        return state.time;
    }

    return state.next_change;
}
