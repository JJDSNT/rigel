#ifndef SERIAL_H
#define SERIAL_H

typedef struct serial_state {
    unsigned baud_divider;
} serial_state_t;

void serial_reset(serial_state_t *serial);

#endif
