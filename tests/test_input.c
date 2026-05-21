#include "chipset/chipset.h"
#include "rigel/rigel.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    RigelChipset *chipset;

    if (ctx == NULL) {
        return 1;
    }

    chipset = rigel_get_chipset(ctx);
    if (chipset == NULL) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_input_set_joydat(ctx, 0, 0x1234u);
    rigel_input_set_pot_button_x(ctx, 0, true);
    rigel_input_set_pot_button_y(ctx, 0, true);
    rigel_custom_write16(ctx, RIGEL_REG_POTGO, 0x0f00u);

    if (rigel_custom_read16(ctx, RIGEL_REG_JOY0DAT) != 0x1234u) {
        rigel_destroy(ctx);
        return 1;
    }

    if (rigel_custom_read16(ctx, RIGEL_REG_POTGOR) == 0xff00u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_POTGO, 0x0000u);

    if (rigel_custom_read16(ctx, RIGEL_REG_POT0DAT) == 0xffffu) {
        rigel_destroy(ctx);
        return 1;
    }

    if (chipset->paula.input.potgo != 0x0000u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
