#ifndef RIEGEL_CUSTOM_H
#define RIEGEL_CUSTOM_H

#include "riegel_types.h"

enum {
    RIEGEL_CUSTOM_BASE = 0x000,
    RIEGEL_CUSTOM_END = 0x1fe
};

enum {
    RIEGEL_REG_DMACON = 0x096,
    RIEGEL_REG_INTENA = 0x09a,
    RIEGEL_REG_INTREQ = 0x09c,
    RIEGEL_REG_COLOR00 = 0x180
};

enum {
    RIEGEL_SETCLR = 0x8000,
    RIEGEL_DMACON_AUD0EN = 0x0001,
    RIEGEL_DMACON_AUD1EN = 0x0002,
    RIEGEL_DMACON_AUD2EN = 0x0004,
    RIEGEL_DMACON_AUD3EN = 0x0008,
    RIEGEL_DMACON_DSKEN = 0x0010,
    RIEGEL_DMACON_SPREN = 0x0020,
    RIEGEL_DMACON_BLTEN = 0x0040,
    RIEGEL_DMACON_COPEN = 0x0080,
    RIEGEL_DMACON_BPLEN = 0x0100,
    RIEGEL_DMACON_DMAEN = 0x0200
};

typedef enum riegel_custom_domain {
    RIEGEL_DOMAIN_UNKNOWN = 0,
    RIEGEL_DOMAIN_AGNUS,
    RIEGEL_DOMAIN_DENISE,
    RIEGEL_DOMAIN_PAULA
} riegel_custom_domain_t;

bool riegel_custom_is_valid_reg(riegel_u32 addr);
riegel_custom_domain_t riegel_custom_domain_for_reg(riegel_u32 addr);

#endif
