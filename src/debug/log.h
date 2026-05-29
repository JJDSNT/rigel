#ifndef RIGEL_DEBUG_LOG_H
#define RIGEL_DEBUG_LOG_H

#include "rigel/rigel_config.h"

/* Register a host-provided log callback.  When fn is non-NULL all subsequent
 * rigel_log_info calls are routed through it. Pass fn=NULL to restore the
 * build default. RIGEL_ENABLE_STDIO_LOG=OFF makes that default a no-op. */
void rigel_log_set_fn(rigel_log_fn_t fn, void *opaque);
void rigel_log_set_event_fn(rigel_log_event_fn_t fn, void *opaque);

void rigel_log_info(const char *message);
void rigel_log_event(const rigel_log_event_t *event);

#endif
