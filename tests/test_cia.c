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

    /* One-shot timer A, stop-to-read: the classic "latch $ffff, one-shot
     * start, count, then stop to read elapsed" measurement (as used by the
     * Copperline timing-test / any E-clock stopwatch). Stopping a one-shot
     * timer must HOLD the counter -- it must not reload it from the latch,
     * or every stopped-timer reading collapses to the latch value. */
    {
        CIA sw;
        uint16_t counter;
        uint16_t elapsed;

        cia_init(&sw, CIA_PORT_A);
        cia_write_reg(&sw, CIA_REG_TALO, 0xFFu);
        cia_write_reg(&sw, CIA_REG_TAHI, 0xFFu);
        /* CRA = LOAD | RUNMODE(one-shot) | START */
        cia_write_reg(&sw, CIA_REG_CRA,
                      (uint8_t)(CIA_CRA_LOAD | CIA_CRA_RUNMODE | CIA_CRA_START));

        cia_step(&sw, 100u); /* 100 E-clock ticks elapse */

        /* Stop the one-shot to read it: CRA = RUNMODE, START cleared. */
        cia_write_reg(&sw, CIA_REG_CRA, (uint8_t)CIA_CRA_RUNMODE);

        counter = (uint16_t)(cia_read_reg(&sw, CIA_REG_TALO) & 0xFFu);
        counter |= (uint16_t)((cia_read_reg(&sw, CIA_REG_TAHI) & 0xFFu) << 8);
        elapsed = (uint16_t)~counter; /* elapsed = 0xFFFF - remaining */

        if (elapsed != 100u) {
            return 1;
        }
    }

    return 0;
}
