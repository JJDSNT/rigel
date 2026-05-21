#include "domains/input/input_domain.h"

bool rigel_input_domain_owns_reg(rigel_u32 addr)
{
    switch (addr) {
    case RIGEL_REG_JOY0DAT:
    case RIGEL_REG_JOY1DAT:
    case RIGEL_REG_POT0DAT:
    case RIGEL_REG_POT1DAT:
    case RIGEL_REG_POTGOR:
    case RIGEL_REG_POTGO:
        return true;
    default:
        return false;
    }
}

void rigel_input_domain_reset(input_state_t *input)
{
    input_reset(input);
}

rigel_u16 rigel_input_domain_read_reg(const input_state_t *input, rigel_u32 addr)
{
    switch (addr) {
    case RIGEL_REG_JOY0DAT:
        return input_read_joydat(input, 0);
    case RIGEL_REG_JOY1DAT:
        return input_read_joydat(input, 1);
    case RIGEL_REG_POT0DAT:
        return input_read_potdat(input, 0);
    case RIGEL_REG_POT1DAT:
        return input_read_potdat(input, 1);
    case RIGEL_REG_POTGOR:
        return input_read_potgor(input);
    default:
        return 0;
    }
}

void rigel_input_domain_write_reg(input_state_t *input, rigel_u32 addr, rigel_u16 value)
{
    if (addr == RIGEL_REG_POTGO) {
        input_write_potgo(input, value);
    }
}
