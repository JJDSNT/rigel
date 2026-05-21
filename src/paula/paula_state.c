#include "paula/paula_state.h"

#include <stddef.h>

#include "core/riegel_context.h"
#include "chipset/chipset.h"
#include "paula/audio.h"
#include "paula/disk.h"
#include "paula/serial.h"

static void riegel_paula_disk_raise_irq(void *opaque, riegel_u16 mask)
{
    RiegelContext *ctx = (RiegelContext *)opaque;

    if (ctx == NULL) {
        return;
    }

    riegel_chipset_raise_irq_source(&ctx->chipset, mask);
}

void riegel_paula_reset(RiegelPaula *paula)
{
    if (paula == NULL) {
        return;
    }

    riegel_paula_interrupts_reset(&paula->interrupts);
    audio_reset(&paula->audio);
    disk_reset(&paula->disk);
    serial_reset(&paula->serial);
}

void riegel_paula_step(RiegelPaula *paula, riegel_u32 ticks)
{
    if (paula == NULL) {
        return;
    }

    disk_step(&paula->disk, ticks);
}

void riegel_paula_raise_irq(RiegelPaula *paula, riegel_u16 mask)
{
    if (paula == NULL) {
        return;
    }

    riegel_paula_interrupts_raise(&paula->interrupts, mask);
}

void riegel_paula_clear_irq(RiegelPaula *paula, riegel_u16 mask)
{
    if (paula == NULL) {
        return;
    }

    riegel_paula_interrupts_clear(&paula->interrupts, mask);
}

void riegel_paula_set_disk_irq_sink(RiegelPaula *paula, paula_disk_irq_sink_t sink)
{
    if (paula == NULL) {
        return;
    }

    disk_set_irq_sink(&paula->disk, sink);
}

void riegel_paula_set_disk_memory_if(RiegelPaula *paula, riegel_chip_ram_if_t chip_ram)
{
    if (paula == NULL) {
        return;
    }

    disk_set_memory_if(&paula->disk, chip_ram);
}

void riegel_paula_set_disk_inserted(RiegelPaula *paula, int inserted)
{
    if (paula == NULL) {
        return;
    }

    disk_set_inserted(&paula->disk, inserted);
}

paula_disk_irq_sink_t riegel_paula_disk_irq_sink(RiegelContext *ctx)
{
    paula_disk_irq_sink_t sink;

    sink.opaque = ctx;
    sink.raise = riegel_paula_disk_raise_irq;
    return sink;
}
