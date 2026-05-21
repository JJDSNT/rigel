#include "riegel/riegel.h"

int main(void)
{
    riegel_config_t cfg = { 0 };
    RiegelContext *ctx = riegel_create(&cfg);
    RiegelChipset *chipset;
    riegel_snapshot_t snapshot = { 0 };

    if (ctx == NULL) {
        return 1;
    }

    chipset = riegel_get_chipset(ctx);
    if (chipset == NULL || riegel_get_chipset_const(ctx) == NULL) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_step(ctx, 4);
    riegel_take_snapshot(ctx, &snapshot);

    if (snapshot.cycles != 4) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_destroy(ctx);
    return 0;
}
