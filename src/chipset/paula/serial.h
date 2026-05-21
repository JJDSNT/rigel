#ifndef RIGEL_PAULA_SERIAL_H
#define RIGEL_PAULA_SERIAL_H

#include <stdbool.h>

#include "rigel/rigel_types.h"

enum {
    RIGEL_PAULA_SERIAL_INTREQ_RBF = 0x0800u,
    RIGEL_PAULA_SERIAL_INTREQ_TBE = 0x0001u,
    RIGEL_PAULA_SERIAL_TX_FIFO_SIZE = 16
};

typedef void (*rigel_serial_irq_raise_fn)(void *opaque, rigel_u16 mask);

typedef struct rigel_serial_irq_sink {
    void *opaque;
    rigel_serial_irq_raise_fn raise;
} rigel_serial_irq_sink_t;

typedef struct serial_state {
    rigel_serial_irq_sink_t irq;

    rigel_u16 serper;
    rigel_u16 tx_buffer;
    bool tx_buffer_valid;

    rigel_u16 tx_shift_reg;
    bool tx_shift_busy;
    rigel_u32 tx_cycles_remaining;

    rigel_u16 rx_buffer;
    bool rx_buffer_full;
    bool overrun;
    bool rxd_level;

    bool tx_instant;

    rigel_u8 tx_fifo[RIGEL_PAULA_SERIAL_TX_FIFO_SIZE];
    unsigned tx_fifo_head;
    unsigned tx_fifo_tail;
    unsigned tx_fifo_count;
    bool tx_overflow;
} serial_state_t;

void serial_reset(serial_state_t *serial);
void serial_set_irq_sink(serial_state_t *serial, rigel_serial_irq_sink_t sink);
void serial_write_serdat(serial_state_t *serial, rigel_u16 value);
void serial_write_serper(serial_state_t *serial, rigel_u16 value);
rigel_u16 serial_read_serdatr(const serial_state_t *serial);
void serial_step(serial_state_t *serial, rigel_u32 cycles);
void serial_receive_byte(serial_state_t *serial, rigel_u8 byte);
void serial_clear_rbf(serial_state_t *serial);
bool serial_tx_available(const serial_state_t *serial);
bool serial_pop_tx_byte(serial_state_t *serial, rigel_u8 *byte_out);
void serial_set_tx_instant(serial_state_t *serial, bool enabled);

#endif
