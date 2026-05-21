#ifndef RIGEL_PAULA_STATE_H
#define RIGEL_PAULA_STATE_H

#include "paula/audio.h"
#include "paula/disk.h"
#include "paula/input.h"
#include "paula/paula_interrupts.h"
#include "paula/serial.h"

typedef struct RigelPaula {
    RigelPaulaInterrupts interrupts;
    audio_state_t audio;
    disk_state_t disk;
    input_state_t input;
    serial_state_t serial;
} RigelPaula;

void rigel_paula_reset(RigelPaula *paula);
void rigel_paula_step(RigelPaula *paula, rigel_u32 ticks);
void rigel_paula_set_dmacon(RigelPaula *paula, rigel_u16 dmacon);
void rigel_paula_raise_irq(RigelPaula *paula, rigel_u16 mask);
void rigel_paula_clear_irq(RigelPaula *paula, rigel_u16 mask);
void rigel_paula_set_disk_irq_sink(RigelPaula *paula, paula_disk_irq_sink_t sink);
void rigel_paula_set_disk_memory_if(RigelPaula *paula, rigel_chip_ram_if_t chip_ram);
void rigel_paula_set_disk_drive(RigelPaula *paula, FloppyDrive *drive);
void rigel_paula_set_serial_irq_sink(RigelPaula *paula, rigel_serial_irq_sink_t sink);
void rigel_paula_set_joydat(RigelPaula *paula, rigel_u32 port, rigel_u16 value);
void rigel_paula_set_pot_button_x(RigelPaula *paula, rigel_u32 port, bool pressed);
void rigel_paula_set_pot_button_y(RigelPaula *paula, rigel_u32 port, bool pressed);
paula_disk_irq_sink_t rigel_paula_disk_irq_sink(RigelContext *ctx);
rigel_serial_irq_sink_t rigel_paula_serial_irq_sink(RigelContext *ctx);

#endif
