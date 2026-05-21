#ifndef RIGEL_INTERRUPT_DOMAIN_H
#define RIGEL_INTERRUPT_DOMAIN_H

#include <stdbool.h>

#include "rigel/rigel_custom.h"
#include "rigel/rigel_types.h"

enum {
    RIGEL_PAULA_INT_TBE = 0x0001u,
    RIGEL_PAULA_INT_DSKBLK = 0x0002u,
    RIGEL_PAULA_INT_SOFT = 0x0004u,
    RIGEL_PAULA_INT_PORTS = 0x0008u,
    RIGEL_PAULA_INT_COPER = 0x0010u,
    RIGEL_PAULA_INT_VERTB = 0x0020u,
    RIGEL_PAULA_INT_BLIT = 0x0040u,
    RIGEL_PAULA_INT_AUD0 = 0x0080u,
    RIGEL_PAULA_INT_AUD1 = 0x0100u,
    RIGEL_PAULA_INT_AUD2 = 0x0200u,
    RIGEL_PAULA_INT_AUD3 = 0x0400u,
    RIGEL_PAULA_INT_RBF = 0x0800u,
    RIGEL_PAULA_INT_DSKSYN = 0x1000u,
    RIGEL_PAULA_INT_EXTER = 0x2000u,
    RIGEL_PAULA_INT_INTEN = 0x4000u,
    RIGEL_PAULA_INT_SETCLR = 0x8000u
};

typedef struct RigelInterruptDomain {
    rigel_u16 intena;
    rigel_u16 intreq;
    rigel_u8 ipl;
} RigelInterruptDomain;

bool rigel_interrupt_domain_owns_reg(rigel_u32 addr);
void rigel_interrupt_domain_reset(RigelInterruptDomain *irq);
void rigel_interrupt_domain_write_intena(RigelInterruptDomain *irq, rigel_u16 value);
void rigel_interrupt_domain_write_intreq(RigelInterruptDomain *irq, rigel_u16 value);
void rigel_interrupt_domain_raise(RigelInterruptDomain *irq, rigel_u16 mask);
void rigel_interrupt_domain_clear(RigelInterruptDomain *irq, rigel_u16 mask);
rigel_u16 rigel_interrupt_domain_read_intena(const RigelInterruptDomain *irq);
rigel_u16 rigel_interrupt_domain_read_intreq(const RigelInterruptDomain *irq);
rigel_u8 rigel_interrupt_domain_current_ipl(const RigelInterruptDomain *irq);
bool rigel_interrupt_domain_pending(const RigelInterruptDomain *irq, rigel_u16 mask);
rigel_u16 rigel_interrupt_domain_read_reg(const RigelInterruptDomain *irq, rigel_u32 addr);
void rigel_interrupt_domain_write_reg(RigelInterruptDomain *irq, rigel_u32 addr, rigel_u16 value);

#endif
