#include "rigel/rigel.h"
#include "rigel/rigel_cia.h"
#include "cia/cia.h"
#include "chipset/chipset.h"
#include "core/rigel_context.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    RigelChipset *chipset;
    rigel_snapshot_t snapshot = { 0 };

    if (ctx == NULL) {
        return 1;
    }

    chipset = &ctx->chipset;
    if (chipset == NULL || &ctx->chipset == NULL) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_step(ctx, 4);
    rigel_take_snapshot(ctx, &snapshot);

    if (snapshot.cycles != 4) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_step(ctx, chipset->agnus.beam.line_clocks);
    if (rigel_cia_read(ctx, 1u, CIA_REG_TODLO) == 0u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
