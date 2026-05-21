#ifndef RIEGEL_EVENTS_H
#define RIEGEL_EVENTS_H

#include "riegel_types.h"

typedef enum riegel_event_kind {
    RIEGEL_EVENT_NONE = 0,
    RIEGEL_EVENT_RESET,
    RIEGEL_EVENT_IRQ,
    RIEGEL_EVENT_VSYNC,
    RIEGEL_EVENT_HSYNC
} riegel_event_kind_t;

typedef struct riegel_event {
    riegel_event_kind_t kind;
    riegel_u64 timestamp;
    riegel_u32 data;
} riegel_event_t;

#endif
