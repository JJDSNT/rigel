#include "domains/serial/serial_domain.h"

bool rigel_serial_domain_owns_reg(rigel_u32 addr)
{
    return addr == RIGEL_REG_SERDATR || addr == RIGEL_REG_SERDAT || addr == RIGEL_REG_SERPER;
}

void rigel_serial_domain_reset(serial_state_t *serial)
{
    serial_reset(serial);
}

void rigel_serial_domain_step(serial_state_t *serial, rigel_u32 cycles)
{
    serial_step(serial, cycles);
}

rigel_u16 rigel_serial_domain_read_reg(serial_state_t *serial, rigel_u32 addr)
{
    if (addr == RIGEL_REG_SERDATR) {
        rigel_u16 value = serial_read_serdatr(serial);
        serial_clear_rbf(serial);
        return value;
    }

    return 0;
}

void rigel_serial_domain_write_reg(serial_state_t *serial, rigel_u32 addr, rigel_u16 value)
{
    switch (addr) {
    case RIGEL_REG_SERDAT:
        serial_write_serdat(serial, value);
        return;
    case RIGEL_REG_SERPER:
        serial_write_serper(serial, value);
        return;
    default:
        return;
    }
}
