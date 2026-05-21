#include "riegel/riegel.h"

int main(void)
{
    if (riegel_custom_domain_for_reg(RIEGEL_REG_DMACON) != RIEGEL_DOMAIN_AGNUS) {
        return 1;
    }

    if (riegel_custom_domain_for_reg(RIEGEL_REG_INTENA) != RIEGEL_DOMAIN_PAULA) {
        return 1;
    }

    if (riegel_custom_domain_for_reg(RIEGEL_REG_INTREQ) != RIEGEL_DOMAIN_PAULA) {
        return 1;
    }

    if (riegel_custom_domain_for_reg(RIEGEL_REG_COLOR00) != RIEGEL_DOMAIN_DENISE) {
        return 1;
    }

    if (riegel_custom_domain_for_reg(0x002) != RIEGEL_DOMAIN_UNKNOWN) {
        return 1;
    }

    return 0;
}
