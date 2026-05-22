#ifndef RIGEL_TIME_H
#define RIGEL_TIME_H

#include "rigel_events.h"
#include "rigel_types.h"

typedef struct rigel_step_result {
    rigel_cycle_t time;
    rigel_u32 events;
} rigel_step_result_t;

rigel_cycle_t       rigel_get_time(const RigelContext *ctx);
rigel_cycle_t       rigel_get_next_deadline(const RigelContext *ctx);
rigel_step_result_t rigel_step(RigelContext *ctx, rigel_cycle_t cycles);
rigel_step_result_t rigel_step_until(RigelContext *ctx, rigel_cycle_t target_time);

#endif
