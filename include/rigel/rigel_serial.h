#ifndef RIGEL_SERIAL_H
#define RIGEL_SERIAL_H

#include <stdbool.h>

#include "rigel_types.h"

typedef struct RigelContext RigelContext;

/*
 * Inject a byte into Paula's serial receive buffer (simulates incoming RS-232).
 * Raises INTREQ RBF (IRQ level 5) if the receive buffer was empty.
 */
void rigel_serial_receive_byte(RigelContext *ctx, rigel_u8 byte);

/*
 * Returns true if at least one byte is waiting in the TX FIFO
 * (the Amiga has transmitted a byte via SERDAT).
 */
bool rigel_serial_tx_available(const RigelContext *ctx);

/*
 * Pops one byte from the TX FIFO into *byte_out.
 * Returns true on success, false if the FIFO is empty.
 */
bool rigel_serial_pop_tx_byte(RigelContext *ctx, rigel_u8 *byte_out);

/*
 * Enable or disable instant TX mode. When enabled, bytes written to SERDAT
 * are queued immediately without waiting for baud-rate timing. Useful for
 * simulation hosts that do not emulate real-time serial baud rates.
 */
void rigel_serial_set_tx_instant(RigelContext *ctx, bool enabled);

#endif
