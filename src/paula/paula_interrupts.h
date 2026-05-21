#ifndef RIGEL_PAULA_INTERRUPTS_H
#define RIGEL_PAULA_INTERRUPTS_H

#include <stdbool.h>

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

typedef struct RigelPaulaInterrupts {
    rigel_u16 intena;
    rigel_u16 intreq;
    rigel_u8 ipl;
} RigelPaulaInterrupts;

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
