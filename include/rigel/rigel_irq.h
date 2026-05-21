#ifndef RIGEL_IRQ_H
#define RIGEL_IRQ_H

#include "rigel_types.h"

rigel_u16 rigel_get_intreq(const RigelContext *ctx);
rigel_u16 rigel_get_intena(const RigelContext *ctx);
rigel_u8 rigel_get_ipl(const RigelContext *ctx);

#endif
