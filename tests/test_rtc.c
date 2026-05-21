#include <time.h>

#include "rigel/rigel.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    time_t now;

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

    rigel_destroy(ctx);
    return 0;
}
