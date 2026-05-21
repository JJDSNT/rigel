#include "rigel/rigel.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);

    if (ctx == NULL) {
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x0f00);

    if (rigel_custom_read16(ctx, RIGEL_REG_COLOR00) != 0x0f00) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_DMACON, RIGEL_SETCLR | RIGEL_DMACON_DMAEN);
    if (rigel_custom_read16(ctx, RIGEL_REG_DMACON) != RIGEL_DMACON_DMAEN) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_DMACON, RIGEL_DMACON_DMAEN);
    if (rigel_custom_read16(ctx, RIGEL_REG_DMACON) != 0x0000) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
