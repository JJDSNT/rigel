#ifndef RIGEL_DEBUG_LOG_H
#define RIGEL_DEBUG_LOG_H

#include "rigel/rigel_config.h"

/* Register a host-provided log callback.  When fn is non-NULL all subsequent
 * rigel_log_info calls are routed through it instead of fprintf(stderr).
 * Pass fn=NULL to restore the default stderr output. */
void rigel_log_set_fn(rigel_log_fn_t fn, void *opaque);

void rigel_log_info(const char *message);

#endif
