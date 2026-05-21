#include "paula/serial.h"

#include <stddef.h>

static rigel_u32 serial_cycles_per_bit(const serial_state_t *serial)
{
    rigel_u32 value = (rigel_u32)(serial->serper & 0x7fffu) + 1u;
    return value != 0 ? value : 1u;
}

static rigel_u32 serial_frame_cycles(const serial_state_t *serial, rigel_u16 word)
{
    (void)word;
    return 10u * serial_cycles_per_bit(serial);
}

static void serial_raise_irq(serial_state_t *serial, rigel_u16 mask)
{
    if (serial != NULL && serial->irq.raise != NULL) {
        serial->irq.raise(serial->irq.opaque, mask);
    }
}

static void serial_queue_tx_byte(serial_state_t *serial, rigel_u16 word)
{
    rigel_u8 byte;

    if (serial == NULL) {
        return;
    }

    byte = (rigel_u8)(word & 0xffu);

    if (serial->tx_fifo_count >= RIGEL_PAULA_SERIAL_TX_FIFO_SIZE) {
        serial->tx_overflow = true;
        return;
    }

    serial->tx_fifo[serial->tx_fifo_tail] = byte;
    serial->tx_fifo_tail = (serial->tx_fifo_tail + 1u) % RIGEL_PAULA_SERIAL_TX_FIFO_SIZE;
    serial->tx_fifo_count++;
}

static void serial_start_tx_shift(serial_state_t *serial)
{
    if (serial == NULL || !serial->tx_buffer_valid || serial->tx_shift_busy) {
        return;
    }

    serial->tx_shift_reg = serial->tx_buffer;
    serial->tx_shift_busy = true;
    serial->tx_cycles_remaining = serial_frame_cycles(serial, serial->tx_shift_reg);
    serial->tx_buffer_valid = false;

    if (serial->tx_instant) {
        serial_queue_tx_byte(serial, serial->tx_shift_reg);
    }

    serial_raise_irq(serial, RIGEL_PAULA_SERIAL_INTREQ_TBE);
}

void serial_reset(serial_state_t *serial)
{
    rigel_serial_irq_sink_t irq;

    if (serial == NULL) {
        return;
    }

    irq = serial->irq;

    serial->serper = 0;
    serial->tx_buffer = 0;
    serial->tx_buffer_valid = false;
    serial->tx_shift_reg = 0;
    serial->tx_shift_busy = false;
    serial->tx_cycles_remaining = 0;
    serial->rx_buffer = 0;
    serial->rx_buffer_full = false;
    serial->overrun = false;
    serial->rxd_level = true;
    serial->tx_instant = false;
    serial->tx_fifo_head = 0;
    serial->tx_fifo_tail = 0;
    serial->tx_fifo_count = 0;
    serial->tx_overflow = false;
    serial->irq = irq;
}

void serial_set_irq_sink(serial_state_t *serial, rigel_serial_irq_sink_t sink)
{
    if (serial == NULL) {
        return;
    }

    serial->irq = sink;
}

void serial_write_serdat(serial_state_t *serial, rigel_u16 value)
{
    if (serial == NULL) {
        return;
    }

    if (serial->tx_instant) {
        serial_queue_tx_byte(serial, value);
        serial_raise_irq(serial, RIGEL_PAULA_SERIAL_INTREQ_TBE);
        return;
    }

    serial->tx_buffer = value;
    serial->tx_buffer_valid = true;

    if (!serial->tx_shift_busy) {
        serial_start_tx_shift(serial);
    }
}

void serial_write_serper(serial_state_t *serial, rigel_u16 value)
{
    if (serial == NULL) {
        return;
    }

    serial->serper = value;
}

rigel_u16 serial_read_serdatr(const serial_state_t *serial)
{
    rigel_u16 value;

    if (serial == NULL) {
        return 0xffffu;
    }

    value = (rigel_u16)(serial->rx_buffer & 0x03ffu);

    if (serial->overrun) {
        value |= 0x8000u;
    }
    if (serial->rx_buffer_full) {
        value |= 0x4000u;
    }
    if (!serial->tx_buffer_valid) {
        value |= 0x2000u;
    }
    if (!serial->tx_shift_busy) {
        value |= 0x1000u;
    }
    if (serial->rxd_level) {
        value |= 0x0800u;
    }

    return value;
}

void serial_step(serial_state_t *serial, rigel_u32 cycles)
{
    rigel_u16 completed_word;

    if (serial == NULL) {
        return;
    }

    if (!serial->tx_shift_busy) {
        if (serial->tx_buffer_valid) {
            serial_start_tx_shift(serial);
        }
        return;
    }

    if (cycles < serial->tx_cycles_remaining) {
        serial->tx_cycles_remaining -= cycles;
        return;
    }

    completed_word = serial->tx_shift_reg;
    serial->tx_cycles_remaining = 0;
    serial->tx_shift_busy = false;
    serial->tx_shift_reg = 0;

    if (!serial->tx_instant) {
        serial_queue_tx_byte(serial, completed_word);
    }

    if (serial->tx_buffer_valid) {
        serial_start_tx_shift(serial);
    }
}

void serial_receive_byte(serial_state_t *serial, rigel_u8 byte)
{
    if (serial == NULL) {
        return;
    }

    if (serial->rx_buffer_full) {
        serial->overrun = true;
    }

    serial->rx_buffer = (rigel_u16)byte | 0x0100u;
    serial->rx_buffer_full = true;
    serial->rxd_level = true;

    serial_raise_irq(serial, RIGEL_PAULA_SERIAL_INTREQ_RBF);
}

void serial_clear_rbf(serial_state_t *serial)
{
    if (serial == NULL) {
        return;
    }

    serial->rx_buffer_full = false;
    serial->rx_buffer = 0;
    serial->overrun = false;
}

bool serial_tx_available(const serial_state_t *serial)
{
    return serial != NULL && serial->tx_fifo_count > 0;
}

bool serial_pop_tx_byte(serial_state_t *serial, rigel_u8 *byte_out)
{
    if (serial == NULL || byte_out == NULL || serial->tx_fifo_count == 0) {
        return false;
    }

    *byte_out = serial->tx_fifo[serial->tx_fifo_head];
    serial->tx_fifo_head = (serial->tx_fifo_head + 1u) % RIGEL_PAULA_SERIAL_TX_FIFO_SIZE;
    serial->tx_fifo_count--;
    return true;
}

void serial_set_tx_instant(serial_state_t *serial, bool enabled)
{
    if (serial == NULL) {
        return;
    }

    serial->tx_instant = enabled;
}
