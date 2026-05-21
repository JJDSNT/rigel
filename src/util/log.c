#include "util/log.h"

#include <stddef.h>
#include <stdio.h>

void rigel_log_info(const char *message)
{
    if (message == NULL) {
        return;
    }

    (void)fprintf(stderr, "[rigel] %s\n", message);
}
