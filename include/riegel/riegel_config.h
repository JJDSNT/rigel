#ifndef RIEGEL_CONFIG_H
#define RIEGEL_CONFIG_H

#include "riegel_types.h"

typedef riegel_u16 (*riegel_chip_ram_read16_fn)(void *opaque, riegel_u32 addr);
typedef void (*riegel_chip_ram_write16_fn)(void *opaque, riegel_u32 addr, riegel_u16 value);

typedef struct riegel_chip_ram_if {
    void *opaque;
    riegel_chip_ram_read16_fn read16;
    riegel_chip_ram_write16_fn write16;
} riegel_chip_ram_if_t;

typedef struct riegel_config {
    riegel_u32 clock_hz;
    riegel_u32 chip_ram_size;
    bool enable_trace;
    riegel_chip_ram_if_t chip_ram;
} riegel_config_t;

#endif
