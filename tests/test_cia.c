#include "cia/cia.h"
#include "cia/cia_interrupt.h"

int main(void)
{
    CIA cia;

    cia_init(&cia, CIA_PORT_A);

    if (cia_port_a_value(&cia) != 0xffu) {
        return 1;
    }

    cia_write_reg(&cia, CIA_REG_TALO, 0x34u);
    cia_write_reg(&cia, CIA_REG_TAHI, 0x12u);

    if (cia.ta_latch != 0x1234u) {
        return 1;
    }

    cia_write_reg(&cia, CIA_REG_ICR, CIA_ICR_SETCLR | CIA_ICR_TA);
    cia_interrupt_raise(&cia, CIA_ICR_TA);

    if (!cia_irq_pending(&cia)) {
        return 1;
    }

    if ((cia_read_reg(&cia, CIA_REG_ICR) & CIA_ICR_IRQ) == 0u) {
        return 1;
    }

    return 0;
}
