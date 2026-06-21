#ifndef RIGEL_CONFIG_H
#define RIGEL_CONFIG_H

#include <time.h>

#include "rigel_rtc.h"
#include "rigel_types.h"

/* Host log callback. When set, Rigel routes all internal log messages through
 * this function instead of writing to stderr. The message is already formatted;
 * the host should add any prefix or newline it needs. */
typedef void (*rigel_log_fn_t)(const char *message, void *opaque);

typedef enum rigel_log_event_id {
    RIGEL_LOG_EVENT_COPPER_WRITE = 1,
    RIGEL_LOG_EVENT_BPL_FETCH    = 2,
    RIGEL_LOG_EVENT_SCHEDULER    = 3,
    RIGEL_LOG_EVENT_COMPOSE      = 4,
    RIGEL_LOG_EVENT_AUDIO_PER_WRITE = 5,
    RIGEL_LOG_EVENT_AUDIO_PERIOD    = 6,
    RIGEL_LOG_EVENT_AUDIO_IRQ       = 7,
    RIGEL_LOG_EVENT_AUDIO_RELOAD    = 8,
    RIGEL_LOG_EVENT_AUDIO_FETCH     = 9,
    RIGEL_LOG_EVENT_AUDIO_DAT_WRITE = 10
} rigel_log_event_id_t;

typedef struct rigel_log_event {
    rigel_log_event_id_t id;
    const char *name;
    rigel_u32 fields[16];
    rigel_u8 field_count;
} rigel_log_event_t;

/* Structured event callback for hosts that want trace data without libc
 * formatting. Field meanings are event-specific and documented next to each
 * internal emission site. */
typedef void (*rigel_log_event_fn_t)(const rigel_log_event_t *event, void *opaque);

typedef enum rigel_video_std {
    RIGEL_VIDEO_NTSC = 0,
    RIGEL_VIDEO_PAL  = 1,
} rigel_video_std_t;

typedef enum rigel_chipset_model {
    RIGEL_CHIPSET_OCS = 0,
    RIGEL_CHIPSET_ECS = 1
} rigel_chipset_model_t;

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

typedef struct rigel_serial_config {
    bool tx_instant; /* bypass baud-rate timing for immediate SERDAT TX */
} rigel_serial_config_t;

typedef struct rigel_config {
    rigel_u32         clock_hz;
    rigel_u32         chip_ram_size;
    bool              enable_trace;
    rigel_chip_ram_if_t chip_ram;
    rigel_serial_config_t serial;
    /*
     * RTC: set rtc_model to RIGEL_RTC_MODEL_NONE to disable.
     * rtc_time: initial Amiga calendar time; 0 = use host system clock.
     */
    rigel_rtc_model_t rtc_model;
    time_t            rtc_time;
    /*
     * Optional host log callback. If non-NULL, Rigel internal log messages are
     * delivered here. When NULL, the default build writes to stderr; configure
     * with RIGEL_ENABLE_STDIO_LOG=OFF to make the default a no-op for bare-metal
     * hosts that provide no C stdio.
     */
    rigel_log_fn_t    log_fn;
    void             *log_opaque;
    rigel_log_event_fn_t log_event_fn;
    void             *log_event_opaque;
    rigel_video_std_t video_std;
    rigel_chipset_model_t chipset_model;
    rigel_pixel_format_t pixel_format;
    rigel_framebuffer_target_t framebuffer;
} rigel_config_t;

#endif
