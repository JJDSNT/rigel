#include "riegel/riegel.h"

int main(void)
{
    riegel_config_t cfg = { 0 };
    RiegelContext *ctx = riegel_create(&cfg);

    if (ctx == NULL) {
        return 1;
    }

    riegel_custom_write16(ctx, RIEGEL_REG_COLOR00, 0x0f00);

    if (riegel_custom_read16(ctx, RIEGEL_REG_COLOR00) != 0x0f00) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_custom_write16(ctx, RIEGEL_REG_DMACON, RIEGEL_SETCLR | RIEGEL_DMACON_DMAEN);
    if (riegel_custom_read16(ctx, RIEGEL_REG_DMACON) != RIEGEL_DMACON_DMAEN) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_custom_write16(ctx, RIEGEL_REG_DMACON, RIEGEL_DMACON_DMAEN);
    if (riegel_custom_read16(ctx, RIEGEL_REG_DMACON) != 0x0000) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_destroy(ctx);
    return 0;
}
