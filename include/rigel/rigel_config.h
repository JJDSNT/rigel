#ifndef RIGEL_CONFIG_H
#define RIGEL_CONFIG_H

#include <time.h>

#include "rigel_rtc.h"
#include "rigel_types.h"

/* Host log callback. When set, Rigel routes all internal log messages through
 * this function instead of writing to stderr. The message is already formatted;
 * the host should add any prefix or newline it needs. */
typedef void (*rigel_log_fn_t)(const char *message, void *opaque);

typedef enum rigel_video_std {
    RIGEL_VIDEO_NTSC = 0,
    RIGEL_VIDEO_PAL  = 1,
} rigel_video_std_t;

typedef enum rigel_pixel_format {
    RIGEL_PIXEL_RGBA8888 = 0,
    RIGEL_PIXEL_RGB565   = 1
} rigel_pixel_format_t;

typedef struct rigel_framebuffer_target {
    void *pixels;
    rigel_u32 width;
    rigel_u32 height;
    rigel_u32 pitch;              /* bytes between row starts */
    rigel_pixel_format_t format;
    bool little_endian;           /* applies to RGB565 stores */
} rigel_framebuffer_target_t;

typedef rigel_u16 (*rigel_chip_ram_read16_fn)(void *opaque, rigel_u32 addr);
typedef void (*rigel_chip_ram_write16_fn)(void *opaque, rigel_u32 addr, rigel_u16 value);

typedef struct rigel_chip_ram_if {
    void *opaque;
    rigel_chip_ram_read16_fn read16;
    rigel_chip_ram_write16_fn write16;
} rigel_chip_ram_if_t;

typedef struct rigel_config {
    rigel_u32         clock_hz;
    rigel_u32         chip_ram_size;
    bool              enable_trace;
    rigel_chip_ram_if_t chip_ram;
    /*
     * RTC: set rtc_model to RIGEL_RTC_MODEL_NONE to disable.
     * rtc_time: initial Amiga calendar time; 0 = use host system clock.
     */
    rigel_rtc_model_t rtc_model;
    time_t            rtc_time;
    /*
     * Optional host log callback. If non-NULL, Rigel internal log messages are
     * delivered here instead of fprintf(stderr). Useful on bare-metal hosts that
     * have no stderr but have a serial or kprintf sink.
     */
    rigel_log_fn_t    log_fn;
    void             *log_opaque;
    rigel_video_std_t video_std;
    rigel_pixel_format_t pixel_format;
    rigel_framebuffer_target_t framebuffer;
} rigel_config_t;

#endif
