#ifndef RIEGEL_PAULA_STATE_H
#define RIEGEL_PAULA_STATE_H

#include "paula/audio.h"
#include "paula/disk.h"
#include "paula/paula_interrupts.h"
#include "paula/serial.h"

typedef struct RiegelPaula {
    RiegelPaulaInterrupts interrupts;
    audio_state_t audio;
    disk_state_t disk;
    serial_state_t serial;
} RiegelPaula;

void riegel_paula_reset(RiegelPaula *paula);
void riegel_paula_step(RiegelPaula *paula, riegel_u32 ticks);
void riegel_paula_raise_irq(RiegelPaula *paula, riegel_u16 mask);
void riegel_paula_clear_irq(RiegelPaula *paula, riegel_u16 mask);
void riegel_paula_set_disk_irq_sink(RiegelPaula *paula, paula_disk_irq_sink_t sink);
void riegel_paula_set_disk_memory_if(RiegelPaula *paula, riegel_chip_ram_if_t chip_ram);
void riegel_paula_set_disk_inserted(RiegelPaula *paula, int inserted);
paula_disk_irq_sink_t riegel_paula_disk_irq_sink(RiegelContext *ctx);

#endif
