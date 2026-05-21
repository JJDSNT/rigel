#include "paula/paula_interrupts.h"

static rigel_u8 rigel_paula_interrupts_compute_ipl(const RigelPaulaInterrupts *irq)
{
    rigel_u16 pending;

    if (irq == NULL) {
        return 0;
    }

    pending = (rigel_u16)(irq->intreq & irq->intena);

    if ((irq->intena & RIGEL_PAULA_INT_INTEN) == 0) {
        return 0;
    }

    if ((pending & RIGEL_PAULA_INT_EXTER) != 0) {
        return 6;
    }

    if ((pending & (RIGEL_PAULA_INT_DSKSYN | RIGEL_PAULA_INT_RBF)) != 0) {
        return 5;
    }

    if ((pending & (RIGEL_PAULA_INT_AUD0 | RIGEL_PAULA_INT_AUD1 |
                    RIGEL_PAULA_INT_AUD2 | RIGEL_PAULA_INT_AUD3)) != 0) {
        return 4;
    }

    if ((pending & (RIGEL_PAULA_INT_BLIT | RIGEL_PAULA_INT_VERTB |
                    RIGEL_PAULA_INT_COPER)) != 0) {
        return 3;
    }

    if ((pending & RIGEL_PAULA_INT_PORTS) != 0) {
        return 2;
    }

    if ((pending & (RIGEL_PAULA_INT_TBE | RIGEL_PAULA_INT_DSKBLK |
                    RIGEL_PAULA_INT_SOFT)) != 0) {
        return 1;
    }

    return 0;
}

static void rigel_paula_interrupts_update(RigelPaulaInterrupts *irq)
{
    if (irq == NULL) {
        return;
    }

    irq->ipl = rigel_paula_interrupts_compute_ipl(irq);
}

void rigel_paula_interrupts_reset(RigelPaulaInterrupts *irq)
{
    if (irq == NULL) {
        return;
    }

    irq->intena = 0;
    irq->intreq = 0;
    irq->ipl = 0;
}

void rigel_paula_interrupts_write_intena(RigelPaulaInterrupts *irq, rigel_u16 value)
{
    rigel_u16 mask;

    if (irq == NULL) {
        return;
    }

    mask = (rigel_u16)(value & 0x7fffU);

    if ((value & RIGEL_PAULA_INT_SETCLR) != 0) {
        irq->intena = (rigel_u16)(irq->intena | mask);
    } else {
        irq->intena = (rigel_u16)(irq->intena & (rigel_u16)(~mask));
    }

    rigel_paula_interrupts_update(irq);
}

void rigel_paula_interrupts_write_intreq(RigelPaulaInterrupts *irq, rigel_u16 value)
{
    rigel_u16 mask;

    if (irq == NULL) {
        return;
    }

    mask = (rigel_u16)(value & 0x7fffU);

    if ((value & RIGEL_PAULA_INT_SETCLR) != 0) {
        irq->intreq = (rigel_u16)(irq->intreq | mask);
    } else {
        irq->intreq = (rigel_u16)(irq->intreq & (rigel_u16)(~mask));
    }

    rigel_paula_interrupts_update(irq);
}

void rigel_paula_interrupts_raise(RigelPaulaInterrupts *irq, rigel_u16 mask)
{
    if (irq == NULL) {
        return;
    }

    irq->intreq = (rigel_u16)(irq->intreq | (mask & 0x7fffU));
    rigel_paula_interrupts_update(irq);
}

void rigel_paula_interrupts_clear(RigelPaulaInterrupts *irq, rigel_u16 mask)
{
    if (irq == NULL) {
        return;
    }

    irq->intreq = (rigel_u16)(irq->intreq & (rigel_u16)(~(mask & 0x7fffU)));
    rigel_paula_interrupts_update(irq);
}

rigel_u16 rigel_paula_interrupts_read_intena(const RigelPaulaInterrupts *irq)
{
    if (irq == NULL) {
        return 0;
    }

    return irq->intena;
}

rigel_u16 rigel_paula_interrupts_read_intreq(const RigelPaulaInterrupts *irq)
{
    if (irq == NULL) {
        return 0;
    }

    return irq->intreq;
}

rigel_u8 rigel_paula_interrupts_current_ipl(const RigelPaulaInterrupts *irq)
{
    if (irq == NULL) {
        return 0;
    }

    return irq->ipl;
}

bool rigel_paula_interrupts_pending(const RigelPaulaInterrupts *irq, rigel_u16 mask)
{
    if (irq == NULL) {
        return false;
    }

    return (irq->intreq & mask) != 0;
}
