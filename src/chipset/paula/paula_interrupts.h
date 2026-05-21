#ifndef RIGEL_PAULA_INTERRUPTS_H
#define RIGEL_PAULA_INTERRUPTS_H

#include "domains/interrupt/interrupt_domain.h"

typedef RigelInterruptDomain RigelPaulaInterrupts;

void rigel_paula_interrupts_reset(RigelPaulaInterrupts *irq);
void rigel_paula_interrupts_write_intena(RigelPaulaInterrupts *irq, rigel_u16 value);
void rigel_paula_interrupts_write_intreq(RigelPaulaInterrupts *irq, rigel_u16 value);
void rigel_paula_interrupts_raise(RigelPaulaInterrupts *irq, rigel_u16 mask);
void rigel_paula_interrupts_clear(RigelPaulaInterrupts *irq, rigel_u16 mask);
rigel_u16 rigel_paula_interrupts_read_intena(const RigelPaulaInterrupts *irq);
rigel_u16 rigel_paula_interrupts_read_intreq(const RigelPaulaInterrupts *irq);
rigel_u8 rigel_paula_interrupts_current_ipl(const RigelPaulaInterrupts *irq);
bool rigel_paula_interrupts_pending(const RigelPaulaInterrupts *irq, rigel_u16 mask);

#endif
