#include "util/log.h"

#include <stddef.h>
#include <stdio.h>

void riegel_log_info(const char *message)
{
    if (message == NULL) {
        return;
    }

    (void)fprintf(stderr, "[riegel] %s\n", message);
}
