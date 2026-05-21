#include "paula/paula_interrupts.h"

static riegel_u8 riegel_paula_interrupts_compute_ipl(const RiegelPaulaInterrupts *irq)
{
    riegel_u16 pending;

    if (irq == NULL) {
        return 0;
    }

    pending = (riegel_u16)(irq->intreq & irq->intena);

    if ((irq->intena & RIEGEL_PAULA_INT_INTEN) == 0) {
        return 0;
    }

    if ((pending & RIEGEL_PAULA_INT_EXTER) != 0) {
        return 6;
    }

    if ((pending & (RIEGEL_PAULA_INT_DSKSYN | RIEGEL_PAULA_INT_RBF)) != 0) {
        return 5;
    }

    if ((pending & (RIEGEL_PAULA_INT_AUD0 | RIEGEL_PAULA_INT_AUD1 |
                    RIEGEL_PAULA_INT_AUD2 | RIEGEL_PAULA_INT_AUD3)) != 0) {
        return 4;
    }

    if ((pending & (RIEGEL_PAULA_INT_BLIT | RIEGEL_PAULA_INT_VERTB |
                    RIEGEL_PAULA_INT_COPER)) != 0) {
        return 3;
    }

    if ((pending & RIEGEL_PAULA_INT_PORTS) != 0) {
        return 2;
    }

    if ((pending & (RIEGEL_PAULA_INT_TBE | RIEGEL_PAULA_INT_DSKBLK |
                    RIEGEL_PAULA_INT_SOFT)) != 0) {
        return 1;
    }

    return 0;
}

static void riegel_paula_interrupts_update(RiegelPaulaInterrupts *irq)
{
    if (irq == NULL) {
        return;
    }

    irq->ipl = riegel_paula_interrupts_compute_ipl(irq);
}

void riegel_paula_interrupts_reset(RiegelPaulaInterrupts *irq)
{
    if (irq == NULL) {
        return;
    }

    irq->intena = 0;
    irq->intreq = 0;
    irq->ipl = 0;
}

void riegel_paula_interrupts_write_intena(RiegelPaulaInterrupts *irq, riegel_u16 value)
{
    riegel_u16 mask;

    if (irq == NULL) {
        return;
    }

    mask = (riegel_u16)(value & 0x7fffU);

    if ((value & RIEGEL_PAULA_INT_SETCLR) != 0) {
        irq->intena = (riegel_u16)(irq->intena | mask);
    } else {
        irq->intena = (riegel_u16)(irq->intena & (riegel_u16)(~mask));
    }

    riegel_paula_interrupts_update(irq);
}

void riegel_paula_interrupts_write_intreq(RiegelPaulaInterrupts *irq, riegel_u16 value)
{
    riegel_u16 mask;

    if (irq == NULL) {
        return;
    }

    mask = (riegel_u16)(value & 0x7fffU);

    if ((value & RIEGEL_PAULA_INT_SETCLR) != 0) {
        irq->intreq = (riegel_u16)(irq->intreq | mask);
    } else {
        irq->intreq = (riegel_u16)(irq->intreq & (riegel_u16)(~mask));
    }

    riegel_paula_interrupts_update(irq);
}

void riegel_paula_interrupts_raise(RiegelPaulaInterrupts *irq, riegel_u16 mask)
{
    if (irq == NULL) {
        return;
    }

    irq->intreq = (riegel_u16)(irq->intreq | (mask & 0x7fffU));
    riegel_paula_interrupts_update(irq);
}

void riegel_paula_interrupts_clear(RiegelPaulaInterrupts *irq, riegel_u16 mask)
{
    if (irq == NULL) {
        return;
    }

    irq->intreq = (riegel_u16)(irq->intreq & (riegel_u16)(~(mask & 0x7fffU)));
    riegel_paula_interrupts_update(irq);
}

riegel_u16 riegel_paula_interrupts_read_intena(const RiegelPaulaInterrupts *irq)
{
    if (irq == NULL) {
        return 0;
    }

    return irq->intena;
}

riegel_u16 riegel_paula_interrupts_read_intreq(const RiegelPaulaInterrupts *irq)
{
    if (irq == NULL) {
        return 0;
    }

    return irq->intreq;
}

riegel_u8 riegel_paula_interrupts_current_ipl(const RiegelPaulaInterrupts *irq)
{
    if (irq == NULL) {
        return 0;
    }

    return irq->ipl;
}

bool riegel_paula_interrupts_pending(const RiegelPaulaInterrupts *irq, riegel_u16 mask)
{
    if (irq == NULL) {
        return false;
    }

    return (irq->intreq & mask) != 0;
}
