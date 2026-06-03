#include "agnus/blitter/blitter.h"
#include "agnus/agnus_state.h"
#include "rigel/rigel.h"

typedef struct TestChipRam {
    rigel_u16 words[16];
} TestChipRam;

typedef struct TraceChipRam {
    rigel_u32 low_writes;
    rigel_u32 dst_writes;
    rigel_u32 other_writes;
} TraceChipRam;

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

static rigel_u16 trace_chip_ram_read16(void *opaque, rigel_u32 addr)
{
    (void)opaque;
    (void)addr;
    return 0x0000u;
}

static void trace_chip_ram_write16(void *opaque, rigel_u32 addr, rigel_u16 value)
{
    TraceChipRam *ram = (TraceChipRam *)opaque;

    (void)value;

    if (addr >= 0x000700u && addr < 0x000712u) {
        ram->low_writes++;
    } else if (addr >= 0x016608u && addr < 0x019508u) {
        ram->dst_writes++;
    } else {
        ram->other_writes++;
    }
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

    /* Slot-level scheduling: blitter only runs on FREE bus slots.
     * At vpos=0 (VBL), hpos=1,3,5 are REFRESH. hpos=2,4 are FREE.
     * Beam is at hpos=1 after the first step; step 4 more to pass a free slot. */
    rigel_agnus_step(ctx, 4);

    if ((rigel_custom_read16(ctx, 0x002) & AGNUS_DMACON_BBUSY) != 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if ((rigel_get_intreq(ctx) & 0x0040u) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);

    ram = (TestChipRam){ 0 };
    ctx = rigel_create(&cfg);
    if (ctx == NULL) {
        return 1;
    }

    rigel_custom_write16(ctx, AGNUS_BLTCON0, 0x00f0); /* minterm: D = A bitmask */
    rigel_custom_write16(ctx, AGNUS_BLTCON1, 0x0001); /* line mode, octant 0 */
    rigel_custom_write16(ctx, AGNUS_BLTBDAT, 0xffff);
    rigel_custom_write16(ctx, AGNUS_BLTCPTH, 0x0000);
    rigel_custom_write16(ctx, AGNUS_BLTCPTL, 0x0000);
    rigel_custom_write16(ctx, AGNUS_BLTAMOD, 0x0000);
    rigel_custom_write16(ctx, AGNUS_BLTBMOD, 0x0000);
    rigel_custom_write16(ctx, AGNUS_BLTCMOD, 0x0000);
    rigel_custom_write16(ctx, AGNUS_BLTSIZE, 0x0041); /* one line-mode pixel */

    if ((rigel_custom_read16(ctx, 0x002) & AGNUS_DMACON_BBUSY) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BLTEN
    );

    /* hpos 1,3,5 are refresh. hpos 2 is the first free blitter slot. */
    {
        rigel_step_result_t line_result = rigel_step(ctx, 3);
        if ((line_result.events & RIGEL_EVENT_BLIT_DONE) == 0) {
            rigel_destroy(ctx);
            return 1;
        }
    }

    if ((rigel_custom_read16(ctx, 0x002) & AGNUS_DMACON_BBUSY) != 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if ((rigel_get_intreq(ctx) & 0x0040u) == 0 || ram.words[0] != 0x8000u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);

    {
        TraceChipRam trace_ram = { 0 };
        rigel_config_t trace_cfg = { 0 };

        trace_cfg.chip_ram.opaque = &trace_ram;
        trace_cfg.chip_ram.read16 = trace_chip_ram_read16;
        trace_cfg.chip_ram.write16 = trace_chip_ram_write16;

        ctx = rigel_create(&trace_cfg);
        if (ctx == NULL) {
            return 1;
        }

        rigel_custom_write16(
            ctx,
            RIGEL_REG_DMACON,
            RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BLTEN
        );

        rigel_custom_write16(ctx, AGNUS_BLTAFWM, 0xffffu);
        rigel_custom_write16(ctx, AGNUS_BLTALWM, 0xffffu);
        rigel_custom_write16(ctx, AGNUS_BLTBMOD, 0x0000u);
        rigel_custom_write16(ctx, AGNUS_BLTAMOD, 0x0000u);
        rigel_custom_write16(ctx, AGNUS_BLTDMOD, 0x0000u);
        rigel_custom_write16(ctx, AGNUS_BLTCDAT, 0x5555u);
        rigel_custom_write16(ctx, AGNUS_BLTCON0, 0x05ccu);
        rigel_custom_write16(ctx, AGNUS_BLTCON1, 0x0000u);
        rigel_custom_write16(ctx, AGNUS_BLTBPTH, 0x0001u);
        rigel_custom_write16(ctx, AGNUS_BLTBPTL, 0x6c84u);
        rigel_custom_write16(ctx, AGNUS_BLTDPTH, 0x0001u);
        rigel_custom_write16(ctx, AGNUS_BLTDPTL, 0x6608u);
        rigel_custom_write16(ctx, AGNUS_BLTSIZE, 0x2ee0u);

        rigel_agnus_step(ctx, 20000u);

        if (trace_ram.low_writes != 0u || trace_ram.dst_writes == 0u) {
            rigel_destroy(ctx);
            return 1;
        }

        rigel_destroy(ctx);
    }

    return 0;
}
