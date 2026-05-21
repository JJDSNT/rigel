#include "rigel/rigel.h"

int main(void)
{
    if (!rigel_custom_is_valid_reg(RIGEL_REG_DMACON)) {
        return 1;
    }

    if (rigel_custom_is_valid_reg(0x001)) {
        return 1;
    }

    if (rigel_custom_is_valid_reg(0x200)) {
        return 1;
    }

    if (rigel_custom_domain_for_reg(RIGEL_REG_DMACON) != RIGEL_DOMAIN_AGNUS) {
        return 1;
    }

    if (rigel_custom_domain_for_reg(RIGEL_REG_INTENA) != RIGEL_DOMAIN_PAULA) {
        return 1;
    }

    if (rigel_custom_domain_for_reg(RIGEL_REG_INTREQ) != RIGEL_DOMAIN_PAULA) {
        return 1;
    }

    if (rigel_custom_domain_for_reg(RIGEL_REG_SERDATR) != RIGEL_DOMAIN_PAULA) {
        return 1;
    }

    if (rigel_custom_domain_for_reg(RIGEL_REG_SERDAT) != RIGEL_DOMAIN_PAULA) {
        return 1;
    }

    if (rigel_custom_domain_for_reg(RIGEL_REG_SERPER) != RIGEL_DOMAIN_PAULA) {
        return 1;
    }

    if (rigel_custom_domain_for_reg(RIGEL_REG_JOY0DAT) != RIGEL_DOMAIN_PAULA) {
        return 1;
    }

    if (rigel_custom_domain_for_reg(RIGEL_REG_POTGO) != RIGEL_DOMAIN_PAULA) {
        return 1;
    }

    if (rigel_custom_domain_for_reg(RIGEL_REG_COLOR00) != RIGEL_DOMAIN_DENISE) {
        return 1;
    }

    if (rigel_custom_domain_for_reg(0x002) != RIGEL_DOMAIN_AGNUS) {
        return 1;
    }

    if (rigel_custom_domain_for_reg(0x004) != RIGEL_DOMAIN_UNKNOWN) {
        return 1;
    }

    return 0;
}
