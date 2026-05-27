#include "debug/log.h"

#include <stddef.h>
#include <stdio.h>

static rigel_log_fn_t s_log_fn     = NULL;
static void          *s_log_opaque = NULL;

void rigel_log_set_fn(rigel_log_fn_t fn, void *opaque)
{
    s_log_fn     = fn;
    s_log_opaque = opaque;
}

void rigel_log_info(const char *message)
{
    if (message == NULL) {
        return;
    }

    if (s_log_fn != NULL) {
        s_log_fn(message, s_log_opaque);
    } else {
        (void)fprintf(stderr, "[rigel] %s\n", message);
    }
}
