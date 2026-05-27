#ifndef RIGEL_EVENTS_H
#define RIGEL_EVENTS_H

#include "rigel_types.h"

typedef enum rigel_event_flags {
    RIGEL_EVENT_NONE        = 0,
    RIGEL_EVENT_DEADLINE    = 1 << 0,
    RIGEL_EVENT_IRQ_CHANGED = 1 << 1,
    /*
     * FRAME_READY fires when the beam wraps to the next frame and Denise swaps
     * its completed internal front buffer. If a framebuffer write target is
     * configured, all visible scanlines for that completed frame have also been
     * written to the target by the time this event is reported.
     */
    RIGEL_EVENT_FRAME_READY = 1 << 2,
    RIGEL_EVENT_AUDIO_READY = 1 << 3,
    RIGEL_EVENT_BUS_CHANGED = 1 << 4,
    RIGEL_EVENT_DMA_CHANGED = 1 << 5,
    RIGEL_EVENT_BLIT_DONE   = 1 << 6,
    RIGEL_EVENT_COPPER      = 1 << 7,
    /*
     * VBLANK fires on the false->true transition of the hardware vertical
     * blank zone, currently Agnus lines 0-25. Because a new frame starts inside
     * VBLANK, FRAME_READY and VBLANK may be reported by the same rigel_step().
     */
    RIGEL_EVENT_VBLANK      = 1 << 8,
    RIGEL_EVENT_HBLANK      = 1 << 9
} rigel_event_flags_t;

#endif
