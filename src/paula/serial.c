#include "paula/serial.h"

#include <stddef.h>

void serial_reset(serial_state_t *serial)
{
    if (serial == NULL) {
        return;
    }

    serial->baud_divider = 0;
}
