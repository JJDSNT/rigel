#ifndef RIGEL_AGNUS_TRACE_H
#define RIGEL_AGNUS_TRACE_H

#include <stdbool.h>
#include "rigel/rigel_types.h"

/* Agnus execution trace — optional structured log of chip activity.
 * All functions are no-ops when tracing is disabled (compile-time or runtime). */

typedef struct agnus_trace_config {
    bool enabled;
    bool copper;
    bool blitter;
    bool beam;
    bool dma;
} agnus_trace_config_t;

void agnus_trace_init(agnus_trace_config_t *cfg);
void agnus_trace_enable(agnus_trace_config_t *cfg, bool on);

void agnus_trace_copper_move(const agnus_trace_config_t *cfg,
                             rigel_u16 addr, rigel_u16 val);
void agnus_trace_copper_wait(const agnus_trace_config_t *cfg,
                             rigel_u16 hpos, rigel_u16 vpos);
void agnus_trace_copper_skip(const agnus_trace_config_t *cfg,
                             rigel_u16 hpos, rigel_u16 vpos, bool taken);
void agnus_trace_blitter_start(const agnus_trace_config_t *cfg,
                               rigel_u32 width, rigel_u32 height);
void agnus_trace_blitter_done(const agnus_trace_config_t *cfg,
                              rigel_u32 cycles);
void agnus_trace_beam(const agnus_trace_config_t *cfg,
                      rigel_u16 hpos, rigel_u16 vpos);

#endif
