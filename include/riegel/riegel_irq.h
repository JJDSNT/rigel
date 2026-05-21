#ifndef RIEGEL_IRQ_H
#define RIEGEL_IRQ_H

#include "riegel_types.h"

riegel_u16 riegel_get_intreq(const RiegelContext *ctx);
riegel_u16 riegel_get_intena(const RiegelContext *ctx);
riegel_u8 riegel_get_ipl(const RiegelContext *ctx);

#endif
