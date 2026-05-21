#include "paula/paula_state.h"

#include <stddef.h>

#include "core/rigel_context.h"
#include "chipset/chipset.h"
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

void rigel_paula_reset(RigelPaula *paula)
{
    if (paula == NULL) {
        return;
    }

    rigel_paula_interrupts_reset(&paula->interrupts);
    audio_reset(&paula->audio);
    disk_reset(&paula->disk);
    serial_reset(&paula->serial);
}

void rigel_paula_step(RigelPaula *paula, rigel_u32 ticks)
{
    if (paula == NULL) {
        return;
    }

    disk_step(&paula->disk, ticks);
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

paula_disk_irq_sink_t rigel_paula_disk_irq_sink(RigelContext *ctx)
{
    paula_disk_irq_sink_t sink;

    sink.opaque = ctx;
    sink.raise = rigel_paula_disk_raise_irq;
    return sink;
}
