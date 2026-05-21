#ifndef RIGEL_CONFIG_H
#define RIGEL_CONFIG_H

#include "rigel_types.h"

typedef rigel_u16 (*rigel_chip_ram_read16_fn)(void *opaque, rigel_u32 addr);
typedef void (*rigel_chip_ram_write16_fn)(void *opaque, rigel_u32 addr, rigel_u16 value);

typedef struct rigel_chip_ram_if {
    void *opaque;
    rigel_chip_ram_read16_fn read16;
    rigel_chip_ram_write16_fn write16;
} rigel_chip_ram_if_t;

typedef struct rigel_config {
    rigel_u32 clock_hz;
    rigel_u32 chip_ram_size;
    bool enable_trace;
    rigel_chip_ram_if_t chip_ram;
} rigel_config_t;

#endif
