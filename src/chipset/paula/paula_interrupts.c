#include "paula/paula_interrupts.h"

void rigel_paula_interrupts_reset(RigelPaulaInterrupts *irq)
{
    rigel_interrupt_domain_reset(irq);
}

void rigel_paula_interrupts_write_intena(RigelPaulaInterrupts *irq, rigel_u16 value)
{
    rigel_interrupt_domain_write_intena(irq, value);
}

void rigel_paula_interrupts_write_intreq(RigelPaulaInterrupts *irq, rigel_u16 value)
{
    rigel_interrupt_domain_write_intreq(irq, value);
}

void rigel_paula_interrupts_raise(RigelPaulaInterrupts *irq, rigel_u16 mask)
{
    rigel_interrupt_domain_raise(irq, mask);
}

void rigel_paula_interrupts_clear(RigelPaulaInterrupts *irq, rigel_u16 mask)
{
    rigel_interrupt_domain_clear(irq, mask);
}

rigel_u16 rigel_paula_interrupts_read_intena(const RigelPaulaInterrupts *irq)
{
    return rigel_interrupt_domain_read_intena(irq);
}

rigel_u16 rigel_paula_interrupts_read_intreq(const RigelPaulaInterrupts *irq)
{
    return rigel_interrupt_domain_read_intreq(irq);
}

rigel_u8 rigel_paula_interrupts_current_ipl(const RigelPaulaInterrupts *irq)
{
    return rigel_interrupt_domain_current_ipl(irq);
}

bool rigel_paula_interrupts_pending(const RigelPaulaInterrupts *irq, rigel_u16 mask)
{
    return rigel_interrupt_domain_pending(irq, mask);
}
