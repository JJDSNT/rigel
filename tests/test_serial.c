#include "paula/serial.h"
#include "chipset/chipset.h"
#include "rigel/rigel.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    RigelChipset *chipset;
    rigel_u8 tx_byte = 0;

    if (ctx == NULL) {
        return 1;
    }

    chipset = rigel_get_chipset(ctx);
    if (chipset == NULL) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_INTENA, 0xc801);
    rigel_custom_write16(ctx, RIGEL_REG_SERPER, 3);
    rigel_custom_write16(ctx, RIGEL_REG_SERDAT, 0x0041);

    if ((rigel_get_intreq(ctx) & RIGEL_PAULA_SERIAL_INTREQ_TBE) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_step(ctx, 100);

    if (!serial_tx_available(&chipset->paula.serial)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!serial_pop_tx_byte(&chipset->paula.serial, &tx_byte) || tx_byte != 0x41u) {
        rigel_destroy(ctx);
        return 1;
    }

    serial_receive_byte(&chipset->paula.serial, 0x55u);
    if ((rigel_get_intreq(ctx) & RIGEL_PAULA_SERIAL_INTREQ_RBF) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if ((rigel_custom_read16(ctx, RIGEL_REG_SERDATR) & 0x00ffu) != 0x55u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
