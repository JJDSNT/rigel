#include "chipset/chipset.h"
#include "core/rigel_context.h"
#include "rigel/rigel.h"
#include "rigel/rigel_events.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    RigelChipset *chipset;

    if (ctx == NULL) {
        return 1;
    }

    chipset = &ctx->chipset;
    if (chipset == NULL) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_AUD0VOL, 64);
    rigel_custom_write16(ctx, RIGEL_REG_AUD0PER, 2);
    rigel_custom_write16(ctx, RIGEL_REG_AUD0DAT, 0x7f00);
    rigel_custom_write16(ctx, RIGEL_REG_DMACON, RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_AUD0EN);

    if ((rigel_step(ctx, 3).events & RIGEL_EVENT_AUDIO_READY) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if ((rigel_step(ctx, 2).events & RIGEL_EVENT_AUDIO_READY) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if (chipset->paula.audio.dmacon != (RIGEL_DMACON_DMAEN | RIGEL_DMACON_AUD0EN)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (chipset->paula.audio.ch[0].audvol != 64 || chipset->paula.audio.ch[0].audper != 2) {
        rigel_destroy(ctx);
        return 1;
    }

    if (chipset->paula.audio.ch[0].current_sample == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if (chipset->paula.audio.mixed_left == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
