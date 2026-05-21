#include "agnus/blitter.h"
#include "riegel/riegel.h"

int main(void)
{
    riegel_config_t cfg = { 0 };
    RiegelContext *ctx = riegel_create(&cfg);

    if (ctx == NULL) {
        return 1;
    }

    riegel_custom_write16(ctx, AGNUS_BLTCON0, 0x0fca);
    riegel_custom_write16(ctx, AGNUS_BLTADAT, 0x1234);

    if (riegel_custom_read16(ctx, AGNUS_BLTCON0) != 0x0fca) {
        riegel_destroy(ctx);
        return 1;
    }

    if (riegel_custom_read16(ctx, AGNUS_BLTADAT) != 0x1234) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_destroy(ctx);
    return 0;
}
