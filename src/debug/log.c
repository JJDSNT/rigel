#include "debug/log.h"

#include <stddef.h>

#ifndef RIGEL_ENABLE_STDIO_LOG
#define RIGEL_ENABLE_STDIO_LOG 1
#endif

#if RIGEL_ENABLE_STDIO_LOG
#include <stdio.h>
#endif

static rigel_log_fn_t s_log_fn     = NULL;
static void          *s_log_opaque = NULL;
static rigel_log_event_fn_t s_log_event_fn     = NULL;
static void                *s_log_event_opaque = NULL;

void rigel_log_set_fn(rigel_log_fn_t fn, void *opaque)
{
    s_log_fn     = fn;
    s_log_opaque = opaque;
}

void rigel_log_set_event_fn(rigel_log_event_fn_t fn, void *opaque)
{
    s_log_event_fn     = fn;
    s_log_event_opaque = opaque;
}

void rigel_log_info(const char *message)
{
    if (message == NULL) {
        return;
    }

    if (s_log_fn != NULL) {
        s_log_fn(message, s_log_opaque);
    }
#if RIGEL_ENABLE_STDIO_LOG
    else {
        (void)fprintf(stderr, "[rigel] %s\n", message);
    }
#endif
}

void rigel_log_event(const rigel_log_event_t *event)
{
    rigel_u8 i;

    if (event == NULL) {
        return;
    }

    if (s_log_event_fn != NULL) {
        s_log_event_fn(event, s_log_event_opaque);
        return;
    }

#if RIGEL_ENABLE_STDIO_LOG
    (void)fprintf(stderr, "[rigel] event=%s", event->name != NULL ? event->name : "unknown");
    for (i = 0; i < event->field_count && i < (rigel_u8)(sizeof(event->fields) / sizeof(event->fields[0])); ++i) {
        (void)fprintf(stderr, " f%u=%08x", (unsigned)i, (unsigned)event->fields[i]);
    }
    (void)fprintf(stderr, "\n");
#else
    (void)i;
#endif
}
