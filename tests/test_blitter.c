#include "agnus/blitter.h"
#include "rigel/rigel.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);

    if (ctx == NULL) {
        return 1;
    }

    rigel_custom_write16(ctx, AGNUS_BLTCON0, 0x0fca);
    rigel_custom_write16(ctx, AGNUS_BLTADAT, 0x1234);

    if (rigel_custom_read16(ctx, AGNUS_BLTCON0) != 0x0fca) {
        rigel_destroy(ctx);
        return 1;
    }

    if (rigel_custom_read16(ctx, AGNUS_BLTADAT) != 0x1234) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
