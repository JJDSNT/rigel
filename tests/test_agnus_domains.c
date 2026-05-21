#include "agnus/agnus_state.h"
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

    rigel_agnus_step(ctx, 4);
    if (chipset->agnus.beam.hpos != 4) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BLTEN
    );

    rigel_agnus_step(ctx, 1);

    if (!chipset->agnus.dma.enabled || !chipset->agnus.dma.blitter_enabled) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_DMACON, RIGEL_DMACON_BLTEN);
    rigel_agnus_step(ctx, 1);

    if (!chipset->agnus.dma.enabled || chipset->agnus.dma.blitter_enabled) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
