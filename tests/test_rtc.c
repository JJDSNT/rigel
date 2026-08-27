#include <time.h>

#include "rigel/rigel.h"

static time_t test_now(void *opaque)
{
    unsigned int *calls = (unsigned int *)opaque;
    (*calls)++;
    return (time_t)1700000000;
}

static bool test_to_calendar(void *opaque, time_t value, struct tm *calendar)
{
    unsigned int *calls = (unsigned int *)opaque;
    (void)value;
    (*calls)++;
    *calendar = (struct tm){ .tm_year = 123, .tm_mon = 10, .tm_mday = 14 };
    return true;
}

static bool test_from_calendar(void *opaque, const struct tm *calendar,
                               time_t *value)
{
    unsigned int *calls = (unsigned int *)opaque;
    (void)calendar;
    (*calls)++;
    *value = (time_t)1700000000;
    return true;
}

int main(void)
{
    rigel_config_t cfg = { 0 };
    unsigned int host_calls = 0;
    RigelContext *ctx;
    time_t now;

    cfg.rtc_host.opaque = &host_calls;
    cfg.rtc_host.now = test_now;
    cfg.rtc_host.to_calendar = test_to_calendar;
    cfg.rtc_host.from_calendar = test_from_calendar;
    ctx = rigel_create(&cfg);

    if (ctx == NULL) {
        return 1;
    }

    if (rigel_rtc_get_model(ctx) != RIGEL_RTC_MODEL_NONE) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_rtc_set_model(ctx, RIGEL_RTC_MODEL_OKI);
    if (rigel_rtc_get_model(ctx) != RIGEL_RTC_MODEL_OKI) {
        rigel_destroy(ctx);
        return 1;
    }

    now = (time_t)1700000000;
    rigel_rtc_set_time(ctx, now);

    if (rigel_rtc_read_reg(ctx, 0xF) != 0x4u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_rtc_write_reg(ctx, 0xD, 0x2u);
    if (rigel_rtc_read_reg(ctx, 0xD) != 0x2u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_reset(ctx);
    if (rigel_rtc_get_model(ctx) != RIGEL_RTC_MODEL_OKI) {
        rigel_destroy(ctx);
        return 1;
    }

    if (host_calls == 0u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
