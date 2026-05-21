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

typedef enum riegel_custom_domain {
    RIEGEL_DOMAIN_UNKNOWN = 0,
    RIEGEL_DOMAIN_AGNUS,
    RIEGEL_DOMAIN_DENISE,
    RIEGEL_DOMAIN_PAULA
} riegel_custom_domain_t;

riegel_custom_domain_t riegel_custom_domain_for_reg(riegel_u32 addr);

#endif
