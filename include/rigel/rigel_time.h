#ifndef RIGEL_TIME_H
#define RIGEL_TIME_H

#include "rigel_events.h"
#include "rigel_types.h"

typedef struct rigel_step_result {
    rigel_cycle_t time;
    rigel_u32 events;
} rigel_step_result_t;

rigel_u32         rigel_get_clock_hz(const RigelContext *ctx);
rigel_u32         rigel_get_line_cycles(const RigelContext *ctx);
rigel_u32         rigel_get_frame_cycles(const RigelContext *ctx);
rigel_u64         rigel_cycles_to_us(rigel_cycle_t cycles, rigel_u32 clock_hz);
rigel_cycle_t     rigel_us_to_cycles(rigel_u64 microseconds, rigel_u32 clock_hz);
rigel_cycle_t       rigel_get_time(const RigelContext *ctx);
rigel_cycle_t       rigel_get_next_deadline(const RigelContext *ctx);
/* Next host-observable event. Unlike rigel_get_next_deadline(), this excludes
 * internal DMA-slot boundaries that rigel_step() already processes itself. */
rigel_cycle_t       rigel_get_next_observable_deadline(const RigelContext *ctx);
rigel_step_result_t rigel_step(RigelContext *ctx, rigel_cycle_t cycles);
rigel_step_result_t rigel_step_until(RigelContext *ctx, rigel_cycle_t target_time);

#endif
