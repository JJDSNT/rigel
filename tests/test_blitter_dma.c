#include "agnus/blitter.h"
#include "agnus/agnus_state.h"
#include "riegel/riegel.h"

typedef struct TestChipRam {
    riegel_u16 words[16];
} TestChipRam;

static riegel_u16 test_chip_ram_read16(void *opaque, riegel_u32 addr)
{
    TestChipRam *ram = (TestChipRam *)opaque;
    riegel_u32 index = (addr >> 1) & 0x0fU;
    return ram->words[index];
}

static void test_chip_ram_write16(void *opaque, riegel_u32 addr, riegel_u16 value)
{
    TestChipRam *ram = (TestChipRam *)opaque;
    riegel_u32 index = (addr >> 1) & 0x0fU;
    ram->words[index] = value;
}

int main(void)
{
    TestChipRam ram = { 0 };
    riegel_config_t cfg = { 0 };
    RiegelContext *ctx;

    cfg.chip_ram.opaque = &ram;
    cfg.chip_ram.read16 = test_chip_ram_read16;
    cfg.chip_ram.write16 = test_chip_ram_write16;

    ctx = riegel_create(&cfg);
    if (ctx == NULL) {
        return 1;
    }

    riegel_custom_write16(ctx, AGNUS_BLTCON0, 0x09f0);
    riegel_custom_write16(ctx, AGNUS_BLTAFWM, 0xffff);
    riegel_custom_write16(ctx, AGNUS_BLTALWM, 0xffff);
    riegel_custom_write16(ctx, AGNUS_BLTADAT, 0x0000);
    riegel_custom_write16(ctx, AGNUS_BLTBDAT, 0x0000);
    riegel_custom_write16(ctx, AGNUS_BLTCDAT, 0x0000);
    riegel_custom_write16(ctx, AGNUS_BLTDPTH, 0x0000);
    riegel_custom_write16(ctx, AGNUS_BLTDPTL, 0x0000);
    riegel_custom_write16(ctx, AGNUS_BLTSIZE, 0x0041);

    if ((riegel_custom_read16(ctx, 0x002) & AGNUS_DMACON_BBUSY) == 0) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_agnus_step(ctx, 1);

    if ((riegel_custom_read16(ctx, 0x002) & AGNUS_DMACON_BBUSY) == 0) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_custom_write16(
        ctx,
        RIEGEL_REG_DMACON,
        RIEGEL_SETCLR | RIEGEL_DMACON_DMAEN | RIEGEL_DMACON_BLTEN
    );

    riegel_agnus_step(ctx, 1);

    if ((riegel_custom_read16(ctx, 0x002) & AGNUS_DMACON_BBUSY) != 0) {
        riegel_destroy(ctx);
        return 1;
    }

    if ((riegel_get_intreq(ctx) & 0x0040u) == 0) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_destroy(ctx);
    return 0;
}
