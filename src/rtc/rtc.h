#include <time.h>

#ifndef RIGEL_INTERNAL_RTC_H
#define RIGEL_INTERNAL_RTC_H

#include "rigel/rigel_rtc.h"

typedef struct RigelRTC {
    rigel_rtc_model_t model;
    rigel_u8 reg[4][16];
    int64_t time_offset;
} RigelRTC;

void rtc_init(RigelRTC *rtc, rigel_rtc_model_t model);
void rtc_reset(RigelRTC *rtc);
void rtc_set_model(RigelRTC *rtc, rigel_rtc_model_t model);
rigel_rtc_model_t rtc_get_model(const RigelRTC *rtc);

time_t rtc_get_time(RigelRTC *rtc);
void rtc_set_time(RigelRTC *rtc, time_t t);
void rtc_update(RigelRTC *rtc);

rigel_u8 rtc_read_reg(RigelRTC *rtc, rigel_u8 nr);
void rtc_write_reg(RigelRTC *rtc, rigel_u8 nr, rigel_u8 value);

rigel_u8 rtc_current_bank(const RigelRTC *rtc);

#endif
