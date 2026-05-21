#ifndef RIGEL_CHIPSET_EVENTS_H
#define RIGEL_CHIPSET_EVENTS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct RigelChipset RigelChipset;

typedef enum rigel_chipset_event_type {
    RIGEL_CHIPSET_EVENT_NONE = 0,
    RIGEL_CHIPSET_EVENT_VBL,
    RIGEL_CHIPSET_EVENT_COPPER,
    RIGEL_CHIPSET_EVENT_BLITTER_DONE,
    RIGEL_CHIPSET_EVENT_AUDIO,
    RIGEL_CHIPSET_EVENT_DISK,
    RIGEL_CHIPSET_EVENT_SERIAL,
    RIGEL_CHIPSET_EVENT_CIA
} rigel_chipset_event_type_t;

typedef struct rigel_chipset_event {
    rigel_chipset_event_type_t type;
    uint32_t flags;
    uint64_t cycle;
} rigel_chipset_event_t;

bool rigel_chipset_event_poll(
    RigelChipset *chipset,
    rigel_chipset_event_t *out
);

#endif
