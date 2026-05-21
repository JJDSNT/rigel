#include "agnus/blitter.h"
#include "agnus/agnus_state.h"
#include "rigel/rigel.h"

typedef struct TestChipRam {
    rigel_u16 words[16];
} TestChipRam;

static rigel_u16 test_chip_ram_read16(void *opaque, rigel_u32 addr)
{
    TestChipRam *ram = (TestChipRam *)opaque;
    rigel_u32 index = (addr >> 1) & 0x0fU;
    return ram->words[index];
}

static void test_chip_ram_write16(void *opaque, rigel_u32 addr, rigel_u16 value)
{
    TestChipRam *ram = (TestChipRam *)opaque;
    rigel_u32 index = (addr >> 1) & 0x0fU;
    ram->words[index] = value;
}

int main(void)
{
    TestChipRam ram = { 0 };
    rigel_config_t cfg = { 0 };
    RigelContext *ctx;

    cfg.chip_ram.opaque = &ram;
    cfg.chip_ram.read16 = test_chip_ram_read16;
    cfg.chip_ram.write16 = test_chip_ram_write16;

    ctx = rigel_create(&cfg);
    if (ctx == NULL) {
        return 1;
    }

    rigel_custom_write16(ctx, AGNUS_BLTCON0, 0x09f0);
    rigel_custom_write16(ctx, AGNUS_BLTAFWM, 0xffff);
    rigel_custom_write16(ctx, AGNUS_BLTALWM, 0xffff);
    rigel_custom_write16(ctx, AGNUS_BLTADAT, 0x0000);
    rigel_custom_write16(ctx, AGNUS_BLTBDAT, 0x0000);
    rigel_custom_write16(ctx, AGNUS_BLTCDAT, 0x0000);
    rigel_custom_write16(ctx, AGNUS_BLTDPTH, 0x0000);
    rigel_custom_write16(ctx, AGNUS_BLTDPTL, 0x0000);
    rigel_custom_write16(ctx, AGNUS_BLTSIZE, 0x0041);

    if ((rigel_custom_read16(ctx, 0x002) & AGNUS_DMACON_BBUSY) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_agnus_step(ctx, 1);

    if ((rigel_custom_read16(ctx, 0x002) & AGNUS_DMACON_BBUSY) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BLTEN
    );

    rigel_agnus_step(ctx, 1);

    if ((rigel_custom_read16(ctx, 0x002) & AGNUS_DMACON_BBUSY) != 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if ((rigel_get_intreq(ctx) & 0x0040u) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
