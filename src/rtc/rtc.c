#include "rtc.h"

#include <assert.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* internal helpers                                                          */
/* ------------------------------------------------------------------------- */

static inline uint8_t rtc_nibble(uint8_t v)
{
    return (uint8_t)(v & 0x0F);
}

static inline uint8_t rtc_bcd_lo(int v)
{
    return (uint8_t)(v % 10);
}

static inline uint8_t rtc_bcd_hi(int v)
{
    return (uint8_t)(v / 10);
}

static inline int rtc_from_bcd(uint8_t lo, uint8_t hi)
{
    return (int)(rtc_nibble(lo) + 10 * rtc_nibble(hi));
}

static time_t rtc_host_now(const RigelRTC *rtc)
{
    if (rtc->host.now != NULL)
        return rtc->host.now(rtc->host.opaque);
#if RIGEL_ENABLE_HOSTED_RTC
    return time(NULL);
#else
    return (time_t)0;
#endif
}

static bool rtc_host_to_calendar(const RigelRTC *rtc, time_t value,
                                 struct tm *out)
{
    if (rtc->host.to_calendar != NULL)
        return rtc->host.to_calendar(rtc->host.opaque, value, out);
#if RIGEL_ENABLE_HOSTED_RTC
#if defined(_WIN32)
    return localtime_s(out, &value) == 0;
#elif defined(_POSIX_VERSION)
    return localtime_r(&value, out) != NULL;
#else
    struct tm *tmp = localtime(&value);
    if (tmp) {
        *out = *tmp;
        return true;
    } else {
        memset(out, 0, sizeof(*out));
        return false;
    }
#endif
#else
    (void)value;
    memset(out, 0, sizeof(*out));
    return false;
#endif
}

static bool rtc_host_from_calendar(const RigelRTC *rtc,
                                   const struct tm *calendar, time_t *value)
{
    if (rtc->host.from_calendar != NULL)
        return rtc->host.from_calendar(rtc->host.opaque, calendar, value);
#if RIGEL_ENABLE_HOSTED_RTC
    {
        struct tm copy = *calendar;
        *value = mktime(&copy);
        return *value != (time_t)-1;
    }
#else
    (void)calendar;
    (void)value;
    return false;
#endif
}

static uint8_t rtc_bank_internal(const RigelRTC *rtc)
{
    return (uint8_t)(rtc->reg[0][0xD] & 0x03);
}

rigel_u8 rtc_current_bank(const RigelRTC *rtc)
{
    return rtc_bank_internal(rtc);
}

static void rtc_apply_model_defaults(RigelRTC *rtc)
{
    memset(rtc->reg, 0, sizeof(rtc->reg));

    switch (rtc->model) {

        case RIGEL_RTC_MODEL_OKI:
            rtc->reg[0][0xD] = 0x1;
            rtc->reg[0][0xE] = 0x0;
            rtc->reg[0][0xF] = 0x4;
            break;

        case RIGEL_RTC_MODEL_RICOH:
            rtc->reg[0][0xD] = 0x8;
            rtc->reg[0][0xE] = 0x0;
            rtc->reg[0][0xF] = 0x0;
            break;

        case RIGEL_RTC_MODEL_NONE:
        default:
            break;
    }
}

static void rtc_time_to_regs_oki(RigelRTC *rtc, const struct tm *t)
{
    int hour = t->tm_hour;
    int pm = 0;
    int is_24h = (rtc->reg[0][0xF] & 0x4) ? 1 : 0;

    rtc->reg[0][0x0] = rtc_bcd_lo(t->tm_sec);
    rtc->reg[0][0x1] = rtc_bcd_hi(t->tm_sec);

    rtc->reg[0][0x2] = rtc_bcd_lo(t->tm_min);
    rtc->reg[0][0x3] = rtc_bcd_hi(t->tm_min);

    if (!is_24h) {
        if (hour == 0) {
            hour = 12;
            pm = 0;
        } else if (hour == 12) {
            pm = 1;
        } else if (hour > 12) {
            hour -= 12;
            pm = 1;
        }
    }

    rtc->reg[0][0x4] = rtc_bcd_lo(hour);
    rtc->reg[0][0x5] = rtc_bcd_hi(hour);
    if (!is_24h && pm)
        rtc->reg[0][0x5] |= 0x4;

    rtc->reg[0][0x6] = rtc_bcd_lo(t->tm_mday);
    rtc->reg[0][0x7] = rtc_bcd_hi(t->tm_mday);

    rtc->reg[0][0x8] = rtc_bcd_lo(t->tm_mon + 1);
    rtc->reg[0][0x9] = rtc_bcd_hi(t->tm_mon + 1);

    rtc->reg[0][0xA] = rtc_bcd_lo(t->tm_year % 100);
    rtc->reg[0][0xB] = rtc_bcd_hi(t->tm_year % 100);

    rtc->reg[0][0xC] = (uint8_t)(t->tm_wday & 0x0F);
}

static void rtc_time_to_regs_ricoh(RigelRTC *rtc, const struct tm *t)
{
    int hour = t->tm_hour;
    int pm = 0;
    int is_24h = (rtc->reg[0][0xA] & 0x1) ? 1 : 0;

    rtc->reg[0][0x0] = rtc_bcd_lo(t->tm_sec);
    rtc->reg[0][0x1] = rtc_bcd_hi(t->tm_sec);

    rtc->reg[0][0x2] = rtc_bcd_lo(t->tm_min);
    rtc->reg[0][0x3] = rtc_bcd_hi(t->tm_min);

    if (!is_24h) {
        if (hour == 0) {
            hour = 12;
            pm = 0;
        } else if (hour == 12) {
            pm = 1;
        } else if (hour > 12) {
            hour -= 12;
            pm = 1;
        }
    }

    rtc->reg[0][0x4] = rtc_bcd_lo(hour);
    rtc->reg[0][0x5] = rtc_bcd_hi(hour);
    if (!is_24h && pm)
        rtc->reg[0][0x5] |= 0x2;

    /*
     * Ricoh layout differs from OKI.
     */
    rtc->reg[0][0x6] = (uint8_t)((t->tm_yday / 7) & 0x0F);
    rtc->reg[0][0x7] = rtc_bcd_lo(t->tm_mday);
    rtc->reg[0][0x8] = rtc_bcd_hi(t->tm_mday);
    rtc->reg[0][0x9] = rtc_bcd_lo(t->tm_mon + 1);
    rtc->reg[0][0xA] = (uint8_t)((rtc->reg[0][0xA] & 0x1) | (rtc_bcd_hi(t->tm_mon + 1) << 1));
    rtc->reg[0][0xB] = rtc_bcd_lo(t->tm_year % 100);
    rtc->reg[0][0xC] = rtc_bcd_hi(t->tm_year % 100);

    /*
     * Keep only the 24h bit in 0xA plus the month high digit encoding above.
     */
    rtc->reg[0][0xA] &= 0x03;

    /*
     * Trim unused bits in alarm bank as a mild nod to the Ricoh layout.
     */
    rtc->reg[1][0x0] &= 0x0;
    rtc->reg[1][0x1] &= 0x0;
    rtc->reg[1][0x2] &= 0xF;
    rtc->reg[1][0x3] &= 0x7;
    rtc->reg[1][0x4] &= 0xF;
    rtc->reg[1][0x5] &= 0x3;
    rtc->reg[1][0x6] &= 0x7;
    rtc->reg[1][0x7] &= 0xF;
    rtc->reg[1][0x8] &= 0x3;
    rtc->reg[1][0x9] &= 0x0;
    rtc->reg[1][0xA] &= 0x1;
    rtc->reg[1][0xB] &= 0x3;
    rtc->reg[1][0xC] &= 0x0;
}

static void rtc_time_to_regs(RigelRTC *rtc)
{
    time_t now;
    struct tm t;

    if (rtc->model == RIGEL_RTC_MODEL_NONE)
        return;

    now = rtc_get_time(rtc);
    if (!rtc_host_to_calendar(rtc, now, &t))
        return;

    switch (rtc->model) {

        case RIGEL_RTC_MODEL_OKI:
            rtc_time_to_regs_oki(rtc, &t);
            break;

        case RIGEL_RTC_MODEL_RICOH:
            rtc_time_to_regs_ricoh(rtc, &t);
            break;

        case RIGEL_RTC_MODEL_NONE:
        default:
            break;
    }
}

static void rtc_regs_to_time_oki(RigelRTC *rtc, struct tm *t)
{
    int hour = rtc_from_bcd(rtc->reg[0][0x4], rtc->reg[0][0x5] & 0x3);
    int is_24h = (rtc->reg[0][0xF] & 0x4) ? 1 : 0;
    int pm = (rtc->reg[0][0x5] & 0x4) ? 1 : 0;

    memset(t, 0, sizeof(*t));

    t->tm_sec  = rtc_from_bcd(rtc->reg[0][0x0], rtc->reg[0][0x1]);
    t->tm_min  = rtc_from_bcd(rtc->reg[0][0x2], rtc->reg[0][0x3]);

    if (!is_24h) {
        if (hour == 12) {
            hour = pm ? 12 : 0;
        } else if (pm) {
            hour += 12;
        }
    }
    t->tm_hour = hour;

    t->tm_mday = rtc_from_bcd(rtc->reg[0][0x6], rtc->reg[0][0x7]);
    t->tm_mon  = rtc_from_bcd(rtc->reg[0][0x8], rtc->reg[0][0x9]) - 1;
    t->tm_year = rtc_from_bcd(rtc->reg[0][0xA], rtc->reg[0][0xB]);

    /*
     * Interpret 00..69 as 2000..2069 and 70..99 as 1970..1999.
     * mktime expects years since 1900.
     */
    t->tm_year += (t->tm_year < 70) ? 100 : 0;
}

static void rtc_regs_to_time_ricoh(RigelRTC *rtc, struct tm *t)
{
    int hour = rtc_from_bcd(rtc->reg[0][0x4], rtc->reg[0][0x5] & 0x1);
    int is_24h = (rtc->reg[0][0xA] & 0x1) ? 1 : 0;
    int pm = (rtc->reg[0][0x5] & 0x2) ? 1 : 0;
    int mon_hi = (rtc->reg[0][0xA] >> 1) & 0x1;

    memset(t, 0, sizeof(*t));

    t->tm_sec  = rtc_from_bcd(rtc->reg[0][0x0], rtc->reg[0][0x1]);
    t->tm_min  = rtc_from_bcd(rtc->reg[0][0x2], rtc->reg[0][0x3]);

    if (!is_24h) {
        if (hour == 12) {
            hour = pm ? 12 : 0;
        } else if (pm) {
            hour += 12;
        }
    }
    t->tm_hour = hour;

    t->tm_mday = rtc_from_bcd(rtc->reg[0][0x7], rtc->reg[0][0x8]);
    t->tm_mon  = (rtc_nibble(rtc->reg[0][0x9]) + 10 * mon_hi) - 1;
    t->tm_year = rtc_from_bcd(rtc->reg[0][0xB], rtc->reg[0][0xC]);

    t->tm_year += (t->tm_year < 70) ? 100 : 0;
}

static void rtc_regs_to_time(RigelRTC *rtc)
{
    struct tm t;
    time_t new_time;

    if (rtc->model == RIGEL_RTC_MODEL_NONE)
        return;

    switch (rtc->model) {

        case RIGEL_RTC_MODEL_OKI:
            rtc_regs_to_time_oki(rtc, &t);
            break;

        case RIGEL_RTC_MODEL_RICOH:
            rtc_regs_to_time_ricoh(rtc, &t);
            break;

        case RIGEL_RTC_MODEL_NONE:
        default:
            return;
    }

    if (rtc_host_from_calendar(rtc, &t, &new_time))
        rtc_set_time(rtc, new_time);
}

static uint8_t rtc_read_special(const RigelRTC *rtc, uint8_t nr)
{
    return rtc_nibble(rtc->reg[0][nr]);
}

static void rtc_write_special(RigelRTC *rtc, uint8_t nr, uint8_t value)
{
    rtc->reg[0][nr] = rtc_nibble(value);
}

/* ------------------------------------------------------------------------- */
/* lifecycle                                                                 */
/* ------------------------------------------------------------------------- */

void rtc_init(RigelRTC *rtc, rigel_rtc_model_t model)
{
    memset(rtc, 0, sizeof(*rtc));
    rtc->model = model;
    rtc_apply_model_defaults(rtc);
    rtc_update(rtc);
}

void rtc_reset(RigelRTC *rtc)
{
    rigel_rtc_model_t model = rtc->model;
    int64_t offset = rtc->time_offset;
    rigel_rtc_host_t host = rtc->host;

    memset(rtc, 0, sizeof(*rtc));
    rtc->model = model;
    rtc->time_offset = offset;
    rtc->host = host;

    rtc_apply_model_defaults(rtc);
    rtc_update(rtc);
}

void rtc_set_host(RigelRTC *rtc, const rigel_rtc_host_t *host)
{
    if (rtc == NULL)
        return;
    if (host != NULL)
        rtc->host = *host;
    else
        memset(&rtc->host, 0, sizeof(rtc->host));
}

void rtc_set_model(RigelRTC *rtc, rigel_rtc_model_t model)
{
    rtc->model = model;
    rtc_apply_model_defaults(rtc);
    rtc_update(rtc);
}

rigel_rtc_model_t rtc_get_model(const RigelRTC *rtc)
{
    return rtc->model;
}

/* ------------------------------------------------------------------------- */
/* time management                                                           */
/* ------------------------------------------------------------------------- */

time_t rtc_get_time(RigelRTC *rtc)
{
    time_t host_now = rtc_host_now(rtc);

    if (rtc->model == RIGEL_RTC_MODEL_NONE)
        return host_now;

    return (time_t)((int64_t)host_now + rtc->time_offset);
}

void rtc_set_time(RigelRTC *rtc, time_t t)
{
    time_t host_now = rtc_host_now(rtc);
    rtc->time_offset = (int64_t)t - (int64_t)host_now;
}

void rtc_update(RigelRTC *rtc)
{
    rtc_time_to_regs(rtc);
}

/* ------------------------------------------------------------------------- */
/* register access                                                           */
/* ------------------------------------------------------------------------- */

rigel_u8 rtc_read_reg(RigelRTC *rtc, rigel_u8 nr)
{
    uint8_t bank;

    assert(nr < 16);

    if (rtc->model == RIGEL_RTC_MODEL_NONE)
        return 0x0;

    rtc_update(rtc);

    switch (nr) {

        case 0xD:
        case 0xE:
        case 0xF:
            return rtc_read_special(rtc, nr);

        default:
            bank = rtc_bank_internal(rtc);
            return rtc_nibble(rtc->reg[bank][nr]);
    }
}

void rtc_write_reg(RigelRTC *rtc, rigel_u8 nr, rigel_u8 value)
{
    uint8_t bank;
    uint8_t nibble;

    assert(nr < 16);

    if (rtc->model == RIGEL_RTC_MODEL_NONE)
        return;

    nibble = rtc_nibble(value);

    switch (nr) {

        case 0xD:
        case 0xE:
        case 0xF:
            rtc_write_special(rtc, nr, nibble);
            return;

        default:
            rtc_update(rtc);
            bank = rtc_bank_internal(rtc);
            rtc->reg[bank][nr] = nibble;

            /*
             * Only bank 0 is interpreted as the live clock image.
             * Other banks are preserved as raw RTC state.
             */
            if (bank == 0)
                rtc_regs_to_time(rtc);
            return;
    }
}
