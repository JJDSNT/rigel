#include "domains/interrupt/interrupt_domain.h"

#include <stdio.h>
#if RIGEL_ENABLE_STDLIB_ENV
#include <stdlib.h>
#endif

static int rigel_irq_trace_enabled(void)
{
#if RIGEL_ENABLE_STDLIB_ENV
    static int enabled = -1;

    if (enabled < 0) {
        const char *env = getenv("RIGEL_IRQ_TRACE");
        enabled = (env != NULL && env[0] != '\0' && env[0] != '0') ? 1 : 0;
    }

    return enabled;
#else
    return 0;
#endif
}

bool rigel_interrupt_domain_owns_reg(rigel_u32 addr)
{
    return addr == RIGEL_REG_INTENA  || addr == RIGEL_REG_INTREQ ||
           addr == RIGEL_REG_INTENAR || addr == RIGEL_REG_INTREQR;
}

static rigel_u8 rigel_interrupt_domain_compute_ipl(const RigelInterruptDomain *irq)
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

static void rigel_interrupt_domain_update(RigelInterruptDomain *irq)
{
    if (irq == NULL) {
        return;
    }

    irq->ipl = rigel_interrupt_domain_compute_ipl(irq);
}

void rigel_interrupt_domain_reset(RigelInterruptDomain *irq)
{
    if (irq == NULL) {
        return;
    }

    irq->intena = 0;
    irq->intreq = 0;
    irq->ipl = 0;
}

void rigel_interrupt_domain_write_intena(RigelInterruptDomain *irq, rigel_u16 value)
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

    rigel_interrupt_domain_update(irq);
    if (rigel_irq_trace_enabled()) {
        fprintf(stderr, "[RIGEL-IRQ-INTENA-W] raw=%04x intena=%04x intreq=%04x ipl=%u\n",
                (unsigned)value, (unsigned)irq->intena, (unsigned)irq->intreq, (unsigned)irq->ipl);
    }
}

void rigel_interrupt_domain_write_intreq(RigelInterruptDomain *irq, rigel_u16 value)
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

    rigel_interrupt_domain_update(irq);
    if (rigel_irq_trace_enabled()) {
        fprintf(stderr, "[RIGEL-IRQ-INTREQ-W] raw=%04x intena=%04x intreq=%04x ipl=%u\n",
                (unsigned)value, (unsigned)irq->intena, (unsigned)irq->intreq, (unsigned)irq->ipl);
    }
}

void rigel_interrupt_domain_raise(RigelInterruptDomain *irq, rigel_u16 mask)
{
    if (irq == NULL) {
        return;
    }

    irq->intreq = (rigel_u16)(irq->intreq | (mask & 0x7fffU));
    rigel_interrupt_domain_update(irq);
    if (rigel_irq_trace_enabled()) {
        fprintf(stderr, "[RIGEL-IRQ-RAISE] mask=%04x intena=%04x intreq=%04x ipl=%u\n",
                (unsigned)mask, (unsigned)irq->intena, (unsigned)irq->intreq, (unsigned)irq->ipl);
    }
}

void rigel_interrupt_domain_clear(RigelInterruptDomain *irq, rigel_u16 mask)
{
    if (irq == NULL) {
        return;
    }

    irq->intreq = (rigel_u16)(irq->intreq & (rigel_u16)(~(mask & 0x7fffU)));
    rigel_interrupt_domain_update(irq);
}

rigel_u16 rigel_interrupt_domain_read_intena(const RigelInterruptDomain *irq)
{
    if (irq == NULL) {
        return 0;
    }

    return irq->intena;
}

rigel_u16 rigel_interrupt_domain_read_intreq(const RigelInterruptDomain *irq)
{
    if (irq == NULL) {
        return 0;
    }

    return irq->intreq;
}

rigel_u8 rigel_interrupt_domain_current_ipl(const RigelInterruptDomain *irq)
{
    if (irq == NULL) {
        return 0;
    }

    return irq->ipl;
}

bool rigel_interrupt_domain_pending(const RigelInterruptDomain *irq, rigel_u16 mask)
{
    if (irq == NULL) {
        return false;
    }

    return (irq->intreq & mask) != 0;
}

rigel_u16 rigel_interrupt_domain_read_reg(const RigelInterruptDomain *irq, rigel_u32 addr)
{
    switch (addr) {
    case RIGEL_REG_INTENA:
    case RIGEL_REG_INTENAR:
        if (rigel_irq_trace_enabled()) {
            fprintf(stderr, "[RIGEL-IRQ-INTENA-R] addr=%03x intena=%04x intreq=%04x ipl=%u\n",
                    (unsigned)addr, (unsigned)irq->intena, (unsigned)irq->intreq, (unsigned)irq->ipl);
        }
        return rigel_interrupt_domain_read_intena(irq);
    case RIGEL_REG_INTREQ:
    case RIGEL_REG_INTREQR:
        if (rigel_irq_trace_enabled()) {
            fprintf(stderr, "[RIGEL-IRQ-INTREQ-R] addr=%03x intena=%04x intreq=%04x ipl=%u\n",
                    (unsigned)addr, (unsigned)irq->intena, (unsigned)irq->intreq, (unsigned)irq->ipl);
        }
        return rigel_interrupt_domain_read_intreq(irq);
    default:
        return 0;
    }
}

void rigel_interrupt_domain_write_reg(RigelInterruptDomain *irq, rigel_u32 addr, rigel_u16 value)
{
    switch (addr) {
    case RIGEL_REG_INTENA:
        rigel_interrupt_domain_write_intena(irq, value);
        return;
    case RIGEL_REG_INTREQ:
        rigel_interrupt_domain_write_intreq(irq, value);
        return;
    default:
        return;
    }
}
