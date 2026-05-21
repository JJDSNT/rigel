#ifndef RIGEL_EVENTS_H
#define RIGEL_EVENTS_H

#include "rigel_types.h"

typedef enum rigel_event_kind {
    RIGEL_EVENT_NONE = 0,
    RIGEL_EVENT_RESET,
    RIGEL_EVENT_IRQ,
    RIGEL_EVENT_VSYNC,
    RIGEL_EVENT_HSYNC
} rigel_event_kind_t;

typedef struct rigel_event {
    rigel_event_kind_t kind;
    rigel_u64 timestamp;
    rigel_u32 data;
} rigel_event_t;

#endif
