#ifndef RIGEL_SERIAL_DOMAIN_H
#define RIGEL_SERIAL_DOMAIN_H

#include "paula/serial.h"
#include "rigel/rigel_custom.h"

bool rigel_serial_domain_owns_reg(rigel_u32 addr);
void rigel_serial_domain_reset(serial_state_t *serial);
void rigel_serial_domain_step(serial_state_t *serial, rigel_u32 cycles);
rigel_u16 rigel_serial_domain_read_reg(serial_state_t *serial, rigel_u32 addr);
void rigel_serial_domain_write_reg(serial_state_t *serial, rigel_u32 addr, rigel_u16 value);

#endif
