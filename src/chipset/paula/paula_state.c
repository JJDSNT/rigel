#include "paula/paula_state.h"

#include <stddef.h>

#include "core/rigel_context.h"
#include "chipset/chipset.h"
#include "domains/audio/audio_domain.h"
#include "domains/disk/disk_domain.h"
#include "domains/input/input_domain.h"
#include "domains/serial/serial_domain.h"
#include "paula/audio.h"
#include "paula/disk.h"
#include "paula/serial.h"

static void rigel_paula_disk_raise_irq(void *opaque, rigel_u16 mask)
{
    RigelContext *ctx = (RigelContext *)opaque;

    if (ctx == NULL) {
        return;
    }

    rigel_chipset_raise_irq_source(&ctx->chipset, mask);
}

static void rigel_paula_serial_raise_irq(void *opaque, rigel_u16 mask)
{
    RigelContext *ctx = (RigelContext *)opaque;

    if (ctx == NULL) {
        return;
    }

    rigel_chipset_raise_irq_source(&ctx->chipset, mask);
}

void rigel_paula_reset(RigelPaula *paula)
{
    if (paula == NULL) {
        return;
    }

    rigel_paula_interrupts_reset(&paula->interrupts);
    rigel_audio_domain_reset(&paula->audio);
    rigel_disk_domain_reset(&paula->disk);
    rigel_input_domain_reset(&paula->input);
    rigel_serial_domain_reset(&paula->serial);
}

void rigel_paula_step(RigelPaula *paula, rigel_u32 ticks)
{
    if (paula == NULL) {
        return;
    }

    rigel_audio_domain_step(&paula->audio, ticks);
    rigel_disk_domain_step(&paula->disk, ticks);
    rigel_serial_domain_step(&paula->serial, ticks);
}

void rigel_paula_set_dmacon(RigelPaula *paula, rigel_u16 dmacon)
{
    if (paula == NULL) {
        return;
    }

    rigel_audio_domain_set_dmacon(&paula->audio, dmacon);
}

void rigel_paula_raise_irq(RigelPaula *paula, rigel_u16 mask)
{
    if (paula == NULL) {
        return;
    }

    rigel_paula_interrupts_raise(&paula->interrupts, mask);
}

void rigel_paula_clear_irq(RigelPaula *paula, rigel_u16 mask)
{
    if (paula == NULL) {
        return;
    }

    rigel_paula_interrupts_clear(&paula->interrupts, mask);
}

void rigel_paula_set_disk_irq_sink(RigelPaula *paula, paula_disk_irq_sink_t sink)
{
    if (paula == NULL) {
        return;
    }

    disk_set_irq_sink(&paula->disk, sink);
}

void rigel_paula_set_disk_memory_if(RigelPaula *paula, rigel_chip_ram_if_t chip_ram)
{
    if (paula == NULL) {
        return;
    }

    disk_set_memory_if(&paula->disk, chip_ram);
}

void rigel_paula_set_disk_drive(RigelPaula *paula, FloppyDrive *drive)
{
    if (paula == NULL) {
        return;
    }

    disk_set_drive(&paula->disk, drive);
}

void rigel_paula_set_serial_irq_sink(RigelPaula *paula, rigel_serial_irq_sink_t sink)
{
    if (paula == NULL) {
        return;
    }

    serial_set_irq_sink(&paula->serial, sink);
}

void rigel_paula_set_joydat(RigelPaula *paula, rigel_u32 port, rigel_u16 value)
{
    if (paula == NULL) {
        return;
    }

    input_set_joydat(&paula->input, port, value);
}

void rigel_paula_set_pot_button_x(RigelPaula *paula, rigel_u32 port, bool pressed)
{
    if (paula == NULL) {
        return;
    }

    input_set_pot_button_x(&paula->input, port, pressed ? 1 : 0);
}

void rigel_paula_set_pot_button_y(RigelPaula *paula, rigel_u32 port, bool pressed)
{
    if (paula == NULL) {
        return;
    }

    input_set_pot_button_y(&paula->input, port, pressed ? 1 : 0);
}

paula_disk_irq_sink_t rigel_paula_disk_irq_sink(RigelContext *ctx)
{
    paula_disk_irq_sink_t sink;

    sink.opaque = ctx;
    sink.raise = rigel_paula_disk_raise_irq;
    return sink;
}

rigel_serial_irq_sink_t rigel_paula_serial_irq_sink(RigelContext *ctx)
{
    rigel_serial_irq_sink_t sink;

    sink.opaque = ctx;
    sink.raise = rigel_paula_serial_raise_irq;
    return sink;
}
