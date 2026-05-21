#include "riegel/riegel.h"

int main(void)
{
    riegel_config_t cfg = { 0 };
    RiegelContext *ctx = riegel_create(&cfg);

    if (ctx == NULL) {
        return 1;
    }

    riegel_custom_write16(ctx, RIEGEL_REG_INTENA, 0x8001);
    riegel_custom_write16(ctx, RIEGEL_REG_INTREQ, 0x8001);

    if (riegel_get_intena(ctx) != 0x0001 || riegel_get_intreq(ctx) != 0x0001) {
        riegel_destroy(ctx);
        return 1;
    }

    if (riegel_get_ipl(ctx) == 0) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_destroy(ctx);
    return 0;
}
