#ifndef RIGEL_EVENTS_H
#define RIGEL_EVENTS_H

#include "rigel_types.h"

typedef enum rigel_event_flags {
    RIGEL_EVENT_NONE        = 0,
    RIGEL_EVENT_DEADLINE    = 1 << 0,
    RIGEL_EVENT_IRQ_CHANGED = 1 << 1,
    RIGEL_EVENT_FRAME_READY = 1 << 2,
    RIGEL_EVENT_AUDIO_READY = 1 << 3,
    RIGEL_EVENT_BUS_CHANGED = 1 << 4,
    RIGEL_EVENT_DMA_CHANGED = 1 << 5,
    RIGEL_EVENT_BLIT_DONE   = 1 << 6,
    RIGEL_EVENT_COPPER      = 1 << 7,
    RIGEL_EVENT_VBLANK      = 1 << 8,
    RIGEL_EVENT_HBLANK      = 1 << 9
} rigel_event_flags_t;

#endif
