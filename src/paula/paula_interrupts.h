#ifndef RIEGEL_PAULA_INTERRUPTS_H
#define RIEGEL_PAULA_INTERRUPTS_H

#include <stdbool.h>

#include "riegel/riegel_types.h"

enum {
    RIEGEL_PAULA_INT_TBE = 0x0001u,
    RIEGEL_PAULA_INT_DSKBLK = 0x0002u,
    RIEGEL_PAULA_INT_SOFT = 0x0004u,
    RIEGEL_PAULA_INT_PORTS = 0x0008u,
    RIEGEL_PAULA_INT_COPER = 0x0010u,
    RIEGEL_PAULA_INT_VERTB = 0x0020u,
    RIEGEL_PAULA_INT_BLIT = 0x0040u,
    RIEGEL_PAULA_INT_AUD0 = 0x0080u,
    RIEGEL_PAULA_INT_AUD1 = 0x0100u,
    RIEGEL_PAULA_INT_AUD2 = 0x0200u,
    RIEGEL_PAULA_INT_AUD3 = 0x0400u,
    RIEGEL_PAULA_INT_RBF = 0x0800u,
    RIEGEL_PAULA_INT_DSKSYN = 0x1000u,
    RIEGEL_PAULA_INT_EXTER = 0x2000u,
    RIEGEL_PAULA_INT_INTEN = 0x4000u,
    RIEGEL_PAULA_INT_SETCLR = 0x8000u
};

typedef struct RiegelPaulaInterrupts {
    riegel_u16 intena;
    riegel_u16 intreq;
    riegel_u8 ipl;
} RiegelPaulaInterrupts;

void riegel_paula_interrupts_reset(RiegelPaulaInterrupts *irq);
void riegel_paula_interrupts_write_intena(RiegelPaulaInterrupts *irq, riegel_u16 value);
void riegel_paula_interrupts_write_intreq(RiegelPaulaInterrupts *irq, riegel_u16 value);
void riegel_paula_interrupts_raise(RiegelPaulaInterrupts *irq, riegel_u16 mask);
void riegel_paula_interrupts_clear(RiegelPaulaInterrupts *irq, riegel_u16 mask);
riegel_u16 riegel_paula_interrupts_read_intena(const RiegelPaulaInterrupts *irq);
riegel_u16 riegel_paula_interrupts_read_intreq(const RiegelPaulaInterrupts *irq);
riegel_u8 riegel_paula_interrupts_current_ipl(const RiegelPaulaInterrupts *irq);
bool riegel_paula_interrupts_pending(const RiegelPaulaInterrupts *irq, riegel_u16 mask);

#endif
