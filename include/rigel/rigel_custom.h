#ifndef RIGEL_CUSTOM_H
#define RIGEL_CUSTOM_H

#include "rigel_types.h"

enum {
    RIGEL_CUSTOM_BASE = 0x000,
    RIGEL_CUSTOM_END = 0x1fe
};

enum {
    RIGEL_REG_DSKDATR = 0x008,
    RIGEL_REG_ADKCONR = 0x010,
    RIGEL_REG_DSKBYTR = 0x01a,
    RIGEL_REG_DSKPTH = 0x020,
    RIGEL_REG_DSKPTL = 0x022,
    RIGEL_REG_DSKLEN = 0x024,
    RIGEL_REG_DMACON = 0x096,
    RIGEL_REG_INTENA = 0x09a,
    RIGEL_REG_ADKCON = 0x09e,
    RIGEL_REG_INTREQ = 0x09c,
    RIGEL_REG_DSKSYNC = 0x07e,
    RIGEL_REG_COLOR00 = 0x180
};

enum {
    RIGEL_SETCLR = 0x8000,
    RIGEL_DMACON_AUD0EN = 0x0001,
    RIGEL_DMACON_AUD1EN = 0x0002,
    RIGEL_DMACON_AUD2EN = 0x0004,
    RIGEL_DMACON_AUD3EN = 0x0008,
    RIGEL_DMACON_DSKEN = 0x0010,
    RIGEL_DMACON_SPREN = 0x0020,
    RIGEL_DMACON_BLTEN = 0x0040,
    RIGEL_DMACON_COPEN = 0x0080,
    RIGEL_DMACON_BPLEN = 0x0100,
    RIGEL_DMACON_DMAEN = 0x0200
};

typedef enum rigel_custom_domain {
    RIGEL_DOMAIN_UNKNOWN = 0,
    RIGEL_DOMAIN_AGNUS,
    RIGEL_DOMAIN_DENISE,
    RIGEL_DOMAIN_PAULA
} rigel_custom_domain_t;

bool rigel_custom_is_valid_reg(rigel_u32 addr);
rigel_custom_domain_t rigel_custom_domain_for_reg(rigel_u32 addr);

#endif
