#ifndef RIGEL_RTC_H
#define RIGEL_RTC_H

#include <time.h>

#include "rigel_types.h"

typedef enum rigel_rtc_model {
    RIGEL_RTC_MODEL_NONE = 0,
    RIGEL_RTC_MODEL_OKI = 1,
    RIGEL_RTC_MODEL_RICOH = 2
} rigel_rtc_model_t;

rigel_rtc_model_t rigel_rtc_get_model(const RigelContext *ctx);
void rigel_rtc_set_model(RigelContext *ctx, rigel_rtc_model_t model);

time_t rigel_rtc_get_time(RigelContext *ctx);
void rigel_rtc_set_time(RigelContext *ctx, time_t value);

rigel_u8 rigel_rtc_read_reg(RigelContext *ctx, rigel_u8 reg);
void rigel_rtc_write_reg(RigelContext *ctx, rigel_u8 reg, rigel_u8 value);

#endif
