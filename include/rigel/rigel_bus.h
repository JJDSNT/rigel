#ifndef RIGEL_BUS_H
#define RIGEL_BUS_H

#include <stdbool.h>

#include "rigel_types.h"

typedef enum rigel_bus_owner {
    RIGEL_BUS_OWNER_NONE,
    RIGEL_BUS_OWNER_CPU,
    RIGEL_BUS_OWNER_REFRESH,
    RIGEL_BUS_OWNER_BITPLANE,
    RIGEL_BUS_OWNER_SPRITE,
    RIGEL_BUS_OWNER_COPPER,
    RIGEL_BUS_OWNER_BLITTER,
    RIGEL_BUS_OWNER_AUDIO,
    RIGEL_BUS_OWNER_DISK,
    RIGEL_BUS_OWNER_CIA,
    RIGEL_BUS_OWNER_CUSTOM
} rigel_bus_owner_t;

typedef enum rigel_bus_dma_flags {
    RIGEL_BUS_DMA_NONE     = 0,
    RIGEL_BUS_DMA_REFRESH  = 1 << 0,
    RIGEL_BUS_DMA_BITPLANE = 1 << 1,
    RIGEL_BUS_DMA_SPRITE   = 1 << 2,
    RIGEL_BUS_DMA_COPPER   = 1 << 3,
    RIGEL_BUS_DMA_BLITTER  = 1 << 4,
    RIGEL_BUS_DMA_AUDIO    = 1 << 5,
    RIGEL_BUS_DMA_DISK     = 1 << 6
} rigel_bus_dma_flags_t;

typedef struct rigel_bus_state {
    rigel_cycle_t     time;
    rigel_bus_owner_t owner;
    rigel_u32         active_dma;
    bool              cpu_can_access_chip_ram;
    bool              cpu_can_access_custom;
    bool              cpu_would_stall;
    rigel_cycle_t     next_change;
} rigel_bus_state_t;

rigel_bus_state_t rigel_get_bus_state(const RigelContext *ctx);
rigel_cycle_t     rigel_get_next_bus_change(const RigelContext *ctx);
bool              rigel_cpu_can_access_chip_ram(const RigelContext *ctx);
bool              rigel_cpu_can_access_custom(const RigelContext *ctx);
rigel_cycle_t     rigel_get_cpu_resume_time(const RigelContext *ctx);

#endif
