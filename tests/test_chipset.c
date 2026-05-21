#include "rigel/rigel.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    RigelChipset *chipset;
    rigel_snapshot_t snapshot = { 0 };

    if (ctx == NULL) {
        return 1;
    }

    chipset = rigel_get_chipset(ctx);
    if (chipset == NULL || rigel_get_chipset_const(ctx) == NULL) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_step(ctx, 4);
    rigel_take_snapshot(ctx, &snapshot);

    if (snapshot.cycles != 4) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
