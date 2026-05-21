#ifndef RIGEL_PAULA_STATE_H
#define RIGEL_PAULA_STATE_H

#include "paula/audio.h"
#include "paula/disk.h"
#include "paula/paula_interrupts.h"
#include "paula/serial.h"

typedef struct RigelPaula {
    RigelPaulaInterrupts interrupts;
    audio_state_t audio;
    disk_state_t disk;
    serial_state_t serial;
} RigelPaula;

void rigel_paula_reset(RigelPaula *paula);
void rigel_paula_step(RigelPaula *paula, rigel_u32 ticks);
void rigel_paula_raise_irq(RigelPaula *paula, rigel_u16 mask);
void rigel_paula_clear_irq(RigelPaula *paula, rigel_u16 mask);
void rigel_paula_set_disk_irq_sink(RigelPaula *paula, paula_disk_irq_sink_t sink);
void rigel_paula_set_disk_memory_if(RigelPaula *paula, rigel_chip_ram_if_t chip_ram);
void rigel_paula_set_disk_drive(RigelPaula *paula, FloppyDrive *drive);
paula_disk_irq_sink_t rigel_paula_disk_irq_sink(RigelContext *ctx);

#endif
