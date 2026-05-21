#include "paula/paula.h"

void paula_irq_raise(Paula *paula, uint16_t mask)
{
    rigel_paula_raise_irq(paula, (rigel_u16)mask);
}

void paula_irq_clear(Paula *paula, uint16_t mask)
{
    rigel_paula_clear_irq(paula, (rigel_u16)mask);
}
