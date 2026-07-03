#include "rigel/rigel.h"
#include "agnus/copper/copper_exec.h"
#include "core/rigel_context.h"

#include <stdio.h>
#include <string.h>

enum {
    TEST_FRAME_CYCLES = 227u * 262u,
    TEST_VISIBLE_Y_START = 26u,
    TEST_VISIBLE_WIDTH = 320u,
    TEST_HIRES_VISIBLE_WIDTH = 640u,
    TEST_LEFT_BORDER = 32u,
    TEST_EXPORTED_WIDTH = TEST_LEFT_BORDER + TEST_VISIBLE_WIDTH,
    TEST_HIRES_EXPORTED_WIDTH = TEST_LEFT_BORDER + TEST_HIRES_VISIBLE_WIDTH,
    TEST_BPL_WORDS = TEST_VISIBLE_WIDTH / 16u,
    TEST_HIRES_BPL_WORDS = TEST_HIRES_VISIBLE_WIDTH / 16u,
    TEST_BPL1_ADDR = 0x0000u,
    TEST_BPL2_ADDR = 0x0040u,
    TEST_HIRES_BPL2_ADDR = 0x0100u,
    TEST_REG_BPLCON0 = 0x100u,
    TEST_REG_BPLCON1 = 0x102u,
    TEST_REG_BPLCON2 = 0x104u,
    TEST_REG_COLOR01 = 0x182u,
    TEST_REG_COLOR02 = 0x184u,
    TEST_REG_COLOR03 = 0x186u,
    TEST_BPL1_PTR_HI = (TEST_BPL1_ADDR >> 16) & 0xffffu,
    TEST_BPL1_PTR_LO = TEST_BPL1_ADDR & 0xffffu,
    TEST_BPL2_PTR_HI = (TEST_BPL2_ADDR >> 16) & 0xffffu,
    TEST_BPL2_PTR_LO = TEST_BPL2_ADDR & 0xffffu
};

static rigel_u16 test_chip_ram_read16(void *opaque, rigel_u32 addr)
{
    rigel_u16 *ram = (rigel_u16 *)opaque;
    return ram[(addr & 0x1ffu) >> 1];
}

static void test_chip_ram_write16(void *opaque, rigel_u32 addr, rigel_u16 value)
{
    rigel_u16 *ram = (rigel_u16 *)opaque;
    ram[(addr & 0x1ffu) >> 1] = value;
}

static void fill_bitplane_words(rigel_u16 *chip_ram, rigel_u16 value)
{
    unsigned i;

    for (i = 0; i < TEST_BPL_WORDS; ++i)
        chip_ram[(TEST_BPL1_ADDR >> 1) + i] = value;
}

static void fill_two_bitplane_words(rigel_u16 *chip_ram, rigel_u16 plane1, rigel_u16 plane2)
{
    unsigned i;

    for (i = 0; i < TEST_BPL_WORDS; ++i) {
        chip_ram[(TEST_BPL1_ADDR >> 1) + i] = plane1;
        chip_ram[(TEST_BPL2_ADDR >> 1) + i] = plane2;
    }
}

static void clear_two_bitplane_words(rigel_u16 *chip_ram)
{
    fill_two_bitplane_words(chip_ram, 0x0000u, 0x0000u);
}

static void setup_lores_single_bitplane(RigelContext *ctx, rigel_u16 ddfstrt, rigel_u16 ddfstop)
{
    rigel_custom_write16(ctx, 0x08e, 0x1a38u);
    rigel_custom_write16(ctx, 0x090, 0x1a78u);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTRT, ddfstrt);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTOP, ddfstop);
    rigel_custom_write16(ctx, RIGEL_REG_BPL1PTH, TEST_BPL1_PTR_HI);
    rigel_custom_write16(ctx, RIGEL_REG_BPL1PTL, TEST_BPL1_PTR_LO);
    rigel_custom_write16(ctx, TEST_REG_BPLCON0, 0x1000u);
    rigel_custom_write16(ctx, TEST_REG_BPLCON1, 0x0000u);
    rigel_custom_write16(ctx, TEST_REG_BPLCON2, 0x0000u);
}

static void setup_lores_two_bitplanes(RigelContext *ctx, rigel_u16 ddfstrt, rigel_u16 ddfstop)
{
    rigel_custom_write16(ctx, 0x08e, 0x1a38u);
    rigel_custom_write16(ctx, 0x090, 0x1a78u);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTRT, ddfstrt);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTOP, ddfstop);
    rigel_custom_write16(ctx, RIGEL_REG_BPL1PTH, TEST_BPL1_PTR_HI);
    rigel_custom_write16(ctx, RIGEL_REG_BPL1PTL, TEST_BPL1_PTR_LO);
    rigel_custom_write16(ctx, RIGEL_REG_BPL2PTH, TEST_BPL2_PTR_HI);
    rigel_custom_write16(ctx, RIGEL_REG_BPL2PTL, TEST_BPL2_PTR_LO);
    rigel_custom_write16(ctx, TEST_REG_BPLCON0, 0x2302u);
    rigel_custom_write16(ctx, TEST_REG_BPLCON1, 0x0000u);
    rigel_custom_write16(ctx, TEST_REG_BPLCON2, 0x0000u);
}

static void setup_lores_dualpf_two_bitplanes(RigelContext *ctx, rigel_u16 ddfstrt, rigel_u16 ddfstop)
{
    setup_lores_two_bitplanes(ctx, ddfstrt, ddfstop);
    rigel_custom_write16(ctx, TEST_REG_BPLCON0, 0x2400u);
}

static void setup_hires_wb13_two_bitplanes(RigelContext *ctx)
{
    rigel_custom_write16(ctx, RIGEL_REG_DIWHIGH, 0x2100u);
    rigel_custom_write16(ctx, RIGEL_REG_DIWSTRT, 0x2b7eu);
    rigel_custom_write16(ctx, RIGEL_REG_DIWSTOP, 0x2bbeu);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTRT, 0x0038u);
    rigel_custom_write16(ctx, RIGEL_REG_DDFSTOP, 0x00d8u);
    rigel_custom_write16(ctx, RIGEL_REG_BPL1PTH, TEST_BPL1_PTR_HI);
    rigel_custom_write16(ctx, RIGEL_REG_BPL1PTL, TEST_BPL1_PTR_LO);
    rigel_custom_write16(ctx, RIGEL_REG_BPL2PTH, (TEST_HIRES_BPL2_ADDR >> 16) & 0xffffu);
    rigel_custom_write16(ctx, RIGEL_REG_BPL2PTL, TEST_HIRES_BPL2_ADDR & 0xffffu);
    rigel_custom_write16(ctx, TEST_REG_BPLCON0, 0xa302u);
    rigel_custom_write16(ctx, TEST_REG_BPLCON1, 0x0055u);
    rigel_custom_write16(ctx, TEST_REG_BPLCON2, 0x0024u);
}

static int test_visual_bitplane_frame(RigelContext *ctx, rigel_u16 *chip_ram)
{
    rigel_frame_t frame;
    const rigel_u32 *pixels;

    rigel_reset(ctx);
    fill_bitplane_words(chip_ram, 0x5555u);
    setup_lores_single_bitplane(ctx, 0x0038u, 0x0060u);
    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x000fu);
    rigel_custom_write16(ctx, TEST_REG_COLOR01, 0x0fffu);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN
    );

    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0) {
        return 1;
    }
    if (!rigel_get_frame(ctx, &frame)) {
        return 1;
    }
    if (frame.width != TEST_EXPORTED_WIDTH || frame.height != 256u) {
        return 1;
    }
    if (frame.pixels == NULL) {
        return 1;
    }
    if (frame.format != RIGEL_PIXEL_RGBA8888 ||
        frame.pitch != RIGEL_DENISE_MAX_SCANLINE_PIXELS * sizeof(rigel_u32)) {
        return 1;
    }
    pixels = (const rigel_u32 *)frame.pixels;
    /* This is intentionally strict: dirty_lines is currently documented as
     * per-raster-line output, not per-quantum/per-frame coarse invalidation. */
    if ((frame.delta.dirty_lines[TEST_VISIBLE_Y_START / 64u] &
         ((rigel_u64)1u << (TEST_VISIBLE_Y_START % 64u))) == 0u) {
        return 1;
    }
    if (pixels[0] != 0x000000ffu ||
        pixels[TEST_LEFT_BORDER + 73u] != 0x000000ffu ||
        pixels[TEST_LEFT_BORDER + 74u] != 0x00ffffffu ||
        pixels[TEST_LEFT_BORDER + 75u] != 0x000000ffu) {
        return 1;
    }

    return 0;
}

static int test_ddfstrt_alignment(RigelContext *ctx, rigel_u16 *chip_ram)
{
    rigel_frame_t frame;
    const rigel_u32 *pixels;

    rigel_reset(ctx);
    fill_bitplane_words(chip_ram, 0xffffu);
    setup_lores_single_bitplane(ctx, 0x0040u, 0x0068u);
    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x000fu);
    rigel_custom_write16(ctx, TEST_REG_COLOR01, 0x0fffu);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN
    );

    if (ctx->chipset.denise.output.ddfstrt_lores != 0x0080u)
        return 1;
    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0)
        return 1;
    if (!rigel_get_frame(ctx, &frame))
        return 1;
    pixels = (const rigel_u32 *)frame.pixels;
    if (pixels[0] != 0x000000ffu)
        return 1;
    if (pixels[TEST_LEFT_BORDER + 88u] != 0x000000ffu)
        return 1;
    if (pixels[TEST_LEFT_BORDER + 89u] != 0x00ffffffu)
        return 1;
    if (pixels[TEST_LEFT_BORDER + 104u] != 0x00ffffffu)
        return 1;

    return 0;
}

static int test_singlepf_bplcon1_scroll(RigelContext *ctx, rigel_u16 *chip_ram)
{
    rigel_frame_t frame;
    const rigel_u32 *pixels;

    rigel_reset(ctx);
    memset(chip_ram, 0, 256u * sizeof(chip_ram[0]));
    chip_ram[TEST_BPL1_ADDR >> 1] = 0x8000u;
    setup_lores_single_bitplane(ctx, 0x0038u, 0x00d0u);
    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x0000u);
    rigel_custom_write16(ctx, TEST_REG_COLOR01, 0x0fffu);
    rigel_custom_write16(ctx, TEST_REG_BPLCON1, 0x0003u);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN
    );

    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0)
        return 1;
    if (!rigel_get_frame(ctx, &frame))
        return 1;
    pixels = (const rigel_u32 *)frame.pixels;

    if (pixels[TEST_LEFT_BORDER + 75u] != 0x00000000u ||
        pixels[TEST_LEFT_BORDER + 76u] != 0x00ffffffu ||
        pixels[TEST_LEFT_BORDER + 77u] != 0x00000000u ||
        pixels[TEST_LEFT_BORDER + 79u] != 0x00000000u) {
        return 1;
    }

    return 0;
}

static int test_singlepf_bplcon1_even_plane_scroll(RigelContext *ctx, rigel_u16 *chip_ram)
{
    rigel_frame_t frame;
    const rigel_u32 *pixels;

    rigel_reset(ctx);
    clear_two_bitplane_words(chip_ram);
    chip_ram[TEST_BPL1_ADDR >> 1] = 0x8000u;
    chip_ram[TEST_BPL2_ADDR >> 1] = 0x8000u;
    setup_lores_two_bitplanes(ctx, 0x0038u, 0x00d0u);
    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x0000u);
    rigel_custom_write16(ctx, TEST_REG_COLOR01, 0x000fu);
    rigel_custom_write16(ctx, TEST_REG_COLOR02, 0x00f0u);
    rigel_custom_write16(ctx, TEST_REG_COLOR03, 0x0fffu);
    rigel_custom_write16(ctx, TEST_REG_BPLCON1, 0x0020u);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN
    );

    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0)
        return 1;
    if (!rigel_get_frame(ctx, &frame))
        return 1;
    pixels = (const rigel_u32 *)frame.pixels;

    if (pixels[TEST_LEFT_BORDER + 73u] != 0x000000ffu ||
        pixels[TEST_LEFT_BORDER + 74u] != 0x00000000u ||
        pixels[TEST_LEFT_BORDER + 75u] != 0x0000ff00u ||
        pixels[TEST_LEFT_BORDER + 76u] != 0x00000000u) {
        return 1;
    }

    return 0;
}

static int test_hires_prefetch_words_are_not_displayed(RigelContext *ctx, rigel_u16 *chip_ram)
{
    rigel_frame_t frame;
    const rigel_u32 *pixels;
    unsigned i;
    unsigned y = TEST_VISIBLE_Y_START;
    unsigned sentinel_x0 = TEST_LEFT_BORDER + 543u;
    unsigned sentinel_x1 = TEST_LEFT_BORDER + 574u;

    rigel_reset(ctx);
    memset(chip_ram, 0, 256u * sizeof(chip_ram[0]));
    for (i = 0; i < TEST_HIRES_BPL_WORDS; ++i) {
        chip_ram[(TEST_BPL1_ADDR >> 1) + i] = 0x0000u;
        chip_ram[(TEST_HIRES_BPL2_ADDR >> 1) + i] = 0x0000u;
    }
    chip_ram[(TEST_BPL1_ADDR >> 1) + TEST_HIRES_BPL_WORDS] = 0xffffu;
    chip_ram[(TEST_BPL1_ADDR >> 1) + TEST_HIRES_BPL_WORDS + 1u] = 0xffffu;
    setup_hires_wb13_two_bitplanes(ctx);
    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x0000u);
    rigel_custom_write16(ctx, TEST_REG_COLOR01, 0x0fffu);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN
    );

    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0)
        return 1;
    if (!rigel_get_frame(ctx, &frame))
        return 1;
    if (frame.width != TEST_HIRES_EXPORTED_WIDTH || frame.height != 256u)
        return 1;
    pixels = (const rigel_u32 *)frame.pixels;

    for (i = sentinel_x0; i <= sentinel_x1; ++i) {
        if (pixels[y * frame.width + i] != 0x00000000u)
            return 1;
    }

    return 0;
}

static int test_bitplane_dma_disable_stops_reuse(RigelContext *ctx, rigel_u16 *chip_ram)
{
    rigel_frame_t frame;
    const rigel_u32 *pixels;

    rigel_reset(ctx);
    fill_bitplane_words(chip_ram, 0xffffu);
    setup_lores_single_bitplane(ctx, 0x0038u, 0x0060u);
    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x000fu);
    rigel_custom_write16(ctx, TEST_REG_COLOR01, 0x0fffu);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN
    );

    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0)
        return 1;
    if (!rigel_get_frame(ctx, &frame))
        return 1;
    pixels = (const rigel_u32 *)frame.pixels;
    if (pixels[TEST_LEFT_BORDER + 88u] != 0x00ffffffu)
        return 1;

    rigel_custom_write16(ctx, RIGEL_REG_DMACON, RIGEL_DMACON_BPLEN);
    fill_bitplane_words(chip_ram, 0x0000u);

    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0)
        return 1;
    if (!rigel_get_frame(ctx, &frame))
        return 1;
    pixels = (const rigel_u32 *)frame.pixels;
    if (pixels[TEST_LEFT_BORDER + 88u] != 0x000000ffu ||
        pixels[TEST_LEFT_BORDER + 89u] != 0x000000ffu)
        return 1;

    return 0;
}

static int test_two_bitplane_fetch_window(RigelContext *ctx, rigel_u16 *chip_ram)
{
    rigel_frame_t frame;
    const rigel_u32 *pixels;

    rigel_reset(ctx);
    fill_two_bitplane_words(chip_ram, 0x5555u, 0x3333u);
    setup_lores_two_bitplanes(ctx, 0x0038u, 0x00d0u);
    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x0000u);
    rigel_custom_write16(ctx, TEST_REG_COLOR01, 0x000fu);
    rigel_custom_write16(ctx, TEST_REG_COLOR02, 0x00f0u);
    rigel_custom_write16(ctx, TEST_REG_COLOR03, 0x0f00u);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN
    );

    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0)
        return 1;
    if (!rigel_get_frame(ctx, &frame))
        return 1;
    pixels = (const rigel_u32 *)frame.pixels;

    /* plane0=0101..., plane1=0011... -> color indices 0,1,2,3 repeating */
    if (pixels[TEST_LEFT_BORDER + 73u] != 0x00000000u ||
        pixels[TEST_LEFT_BORDER + 74u] != 0x000000ffu ||
        pixels[TEST_LEFT_BORDER + 75u] != 0x0000ff00u ||
        pixels[TEST_LEFT_BORDER + 76u] != 0x00ff0000u ||
        pixels[TEST_LEFT_BORDER + 77u] != 0x00000000u ||
        pixels[TEST_LEFT_BORDER + 78u] != 0x000000ffu ||
        pixels[TEST_LEFT_BORDER + 79u] != 0x0000ff00u ||
        pixels[TEST_LEFT_BORDER + 80u] != 0x00ff0000u) {
        return 1;
    }

    if (pixels[TEST_LEFT_BORDER + 319u] != 0x0000ff00u) {
        return 1;
    }

    return 0;
}

static int test_dualpf_bplcon1_independent_scroll(RigelContext *ctx, rigel_u16 *chip_ram)
{
    rigel_frame_t frame;
    const rigel_u32 *pixels;

    rigel_reset(ctx);
    clear_two_bitplane_words(chip_ram);
    chip_ram[TEST_BPL1_ADDR >> 1] = 0x8000u;
    chip_ram[TEST_BPL2_ADDR >> 1] = 0x8000u;
    setup_lores_dualpf_two_bitplanes(ctx, 0x0038u, 0x00d0u);
    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x0000u);
    rigel_custom_write16(ctx, TEST_REG_COLOR01, 0x000fu);
    rigel_custom_write16(ctx, 0x192u, 0x0f00u); /* COLOR09 */
    rigel_custom_write16(ctx, TEST_REG_BPLCON1, 0x0010u);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN
    );

    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0)
        return 1;
    if (!rigel_get_frame(ctx, &frame))
        return 1;
    pixels = (const rigel_u32 *)frame.pixels;

    if (pixels[TEST_LEFT_BORDER + 72u] != 0x00000000u ||
        pixels[TEST_LEFT_BORDER + 73u] != 0x000000ffu ||
        pixels[TEST_LEFT_BORDER + 74u] != 0x00ff0000u) {
        return 1;
    }

    rigel_reset(ctx);
    clear_two_bitplane_words(chip_ram);
    chip_ram[TEST_BPL1_ADDR >> 1] = 0x8000u;
    chip_ram[TEST_BPL2_ADDR >> 1] = 0x8000u;
    setup_lores_dualpf_two_bitplanes(ctx, 0x0038u, 0x00d0u);
    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x0000u);
    rigel_custom_write16(ctx, TEST_REG_COLOR01, 0x000fu);
    rigel_custom_write16(ctx, 0x192u, 0x0f00u); /* COLOR09 */
    rigel_custom_write16(ctx, TEST_REG_BPLCON1, 0x0002u);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN
    );

    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0)
        return 1;
    if (!rigel_get_frame(ctx, &frame))
        return 1;
    pixels = (const rigel_u32 *)frame.pixels;

    if (pixels[TEST_LEFT_BORDER + 73u] != 0x00ff0000u ||
        pixels[TEST_LEFT_BORDER + 74u] != 0x00000000u ||
        pixels[TEST_LEFT_BORDER + 75u] != 0x000000ffu) {
        return 1;
    }

    return 0;
}

static int test_rgb565_frame_format(rigel_u16 *chip_ram)
{
    rigel_config_t cfg = { 0 };
    static rigel_u16 host_framebuffer[256u * TEST_VISIBLE_WIDTH];
    RigelContext *ctx;
    rigel_frame_t frame;
    const rigel_u16 *pixels;
    int result = 0;

    memset(host_framebuffer, 0, sizeof(host_framebuffer));
    cfg.chip_ram.opaque = chip_ram;
    cfg.chip_ram.read16 = test_chip_ram_read16;
    cfg.chip_ram.write16 = test_chip_ram_write16;
    cfg.pixel_format = RIGEL_PIXEL_RGB565;
    cfg.framebuffer.pixels = host_framebuffer;
    cfg.framebuffer.width = TEST_VISIBLE_WIDTH;
    cfg.framebuffer.height = 256u;
    cfg.framebuffer.pitch = TEST_VISIBLE_WIDTH * sizeof(rigel_u16);
    cfg.framebuffer.format = RIGEL_PIXEL_RGB565;
    cfg.framebuffer.little_endian = true;

    ctx = rigel_create(&cfg);
    if (ctx == NULL) {
        return 1;
    }

    fill_bitplane_words(chip_ram, 0x5555u);
    setup_lores_single_bitplane(ctx, 0x0038u, 0x0060u);
    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x000fu);
    rigel_custom_write16(ctx, TEST_REG_COLOR01, 0x0fffu);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN
    );

    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0 ||
        !rigel_get_frame(ctx, &frame)) {
        result = 1;
    } else {
        pixels = (const rigel_u16 *)frame.pixels;
        if (frame.format != RIGEL_PIXEL_RGB565 ||
            frame.pitch != RIGEL_DENISE_MAX_SCANLINE_PIXELS * sizeof(rigel_u16) ||
            pixels == NULL ||
            pixels[TEST_LEFT_BORDER + 73u] != 0x001fu ||
            pixels[TEST_LEFT_BORDER + 74u] != 0xffffu ||
            host_framebuffer[TEST_LEFT_BORDER + 73u] != 0x001fu ||
            host_framebuffer[TEST_LEFT_BORDER + 74u] != 0xffffu) {
            result = 1;
        }
    }

    rigel_destroy(ctx);
    return result;
}

static int check_rgb565_edge_target_bytes(const rigel_u8 *target)
{
    if (target[16] != 0x1fu || target[17] != 0x00u) {
        return 1;
    }

    if (target[40] != 0xaau || target[41] != 0xaau ||
        target[42] != 0xaau || target[43] != 0xaau) {
        return 1;
    }

    return 0;
}

static void setup_rgb565_edge_scene(RigelContext *ctx, rigel_u16 *chip_ram)
{
    memset(chip_ram, 0, 256u * sizeof(chip_ram[0]));
    fill_bitplane_words(chip_ram, 0x5555u);
    setup_lores_single_bitplane(ctx, 0x0000u, 0x0060u);
    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x000fu);
    rigel_custom_write16(ctx, TEST_REG_COLOR01, 0x0fffu);
    rigel_custom_write16(
        ctx,
        RIGEL_REG_DMACON,
        RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BPLEN
    );
}

static int test_rgb565_write_target_edges(rigel_u16 *chip_ram)
{
    rigel_config_t cfg = { 0 };
    static rigel_u8 host_framebuffer[80];
    RigelContext *ctx;
    int result = 0;

    memset(host_framebuffer, 0xaa, sizeof(host_framebuffer));
    cfg.chip_ram.opaque = chip_ram;
    cfg.chip_ram.read16 = test_chip_ram_read16;
    cfg.chip_ram.write16 = test_chip_ram_write16;
    cfg.pixel_format = RIGEL_PIXEL_RGB565;
    cfg.framebuffer.pixels = host_framebuffer;
    cfg.framebuffer.width = 20u;
    cfg.framebuffer.height = 1u;
    cfg.framebuffer.pitch = sizeof(host_framebuffer);
    cfg.framebuffer.format = RIGEL_PIXEL_RGB565;
    cfg.framebuffer.little_endian = true;

    ctx = rigel_create(&cfg);
    if (ctx == NULL) {
        return 1;
    }

    setup_rgb565_edge_scene(ctx, chip_ram);
    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0 ||
        check_rgb565_edge_target_bytes(host_framebuffer) != 0) {
        result = 1;
    }

    if (result == 0) {
        memset(host_framebuffer, 0xaa, sizeof(host_framebuffer));
        rigel_reset(ctx);
        setup_rgb565_edge_scene(ctx, chip_ram);
        if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0 ||
            check_rgb565_edge_target_bytes(host_framebuffer) != 0) {
            result = 1;
        }
    }

    rigel_destroy(ctx);
    return result;
}

static int test_frame_metadata_flags(rigel_u16 *chip_ram)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx;
    rigel_frame_t frame;
    int result = 0;

    cfg.chip_ram.opaque = chip_ram;
    cfg.chip_ram.read16 = test_chip_ram_read16;
    cfg.chip_ram.write16 = test_chip_ram_write16;

    ctx = rigel_create(&cfg);
    if (ctx == NULL) {
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x000fu);
    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0 ||
        !rigel_get_frame(ctx, &frame) ||
        !frame.delta.full_redraw) {
        result = 1;
    }

    if (result == 0 &&
        ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0 ||
         !rigel_get_frame(ctx, &frame) ||
         frame.delta.full_redraw)) {
        result = 1;
    }

    if (result == 0) {
        copper_exec_move(ctx, RIGEL_REG_COLOR00, 0x00f0u);
        if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0 ||
            !rigel_get_frame(ctx, &frame) ||
            (frame.flags & RIGEL_FRAME_COPPER_ACTIVE) == 0u ||
            !frame.delta.full_redraw) {
            result = 1;
        }
    }

    rigel_destroy(ctx);
    return result;
}

static int test_interlace_frame_flags(rigel_u16 *chip_ram)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx;
    rigel_frame_t frame;
    int result = 0;

    cfg.chip_ram.opaque = chip_ram;
    cfg.chip_ram.read16 = test_chip_ram_read16;
    cfg.chip_ram.write16 = test_chip_ram_write16;

    ctx = rigel_create(&cfg);
    if (ctx == NULL) {
        return 1;
    }

    rigel_custom_write16(ctx, TEST_REG_BPLCON0, 0x1004u);

    if ((rigel_step(ctx, (rigel_cycle_t)TEST_FRAME_CYCLES).events & RIGEL_EVENT_FRAME_READY) == 0 ||
        !rigel_get_frame(ctx, &frame) ||
        (frame.flags & RIGEL_FRAME_INTERLACE_EVEN) == 0u ||
        (frame.flags & RIGEL_FRAME_INTERLACE_ODD) != 0u) {
        result = 1;
    }

    if (result == 0 &&
        ((rigel_step(ctx, (rigel_cycle_t)(TEST_FRAME_CYCLES + 227u)).events & RIGEL_EVENT_FRAME_READY) == 0 ||
         !rigel_get_frame(ctx, &frame) ||
         (frame.flags & RIGEL_FRAME_INTERLACE_ODD) == 0u ||
         (frame.flags & RIGEL_FRAME_INTERLACE_EVEN) != 0u)) {
        result = 1;
    }

    rigel_destroy(ctx);
    return result;
}

int main(void)
{
    rigel_config_t cfg = { 0 };
    rigel_u16 chip_ram[256] = { 0 };
    RigelContext *ctx;
    rigel_denise_video_desc_t video;
    rigel_denise_debug_state_t debug;
    rigel_denise_scanline_t scanline;
    rigel_u32 frame_cycles = 227u * 262u;

    cfg.chip_ram.opaque = chip_ram;
    cfg.chip_ram.read16 = test_chip_ram_read16;
    cfg.chip_ram.write16 = test_chip_ram_write16;
    ctx = rigel_create(&cfg);

    if (ctx == NULL) {
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_COLOR00, 0x0f00);
    rigel_custom_write16(ctx, 0x08e, 0x2c81);
    rigel_custom_write16(ctx, 0x090, 0x2cc1);

    if (rigel_custom_read16(ctx, RIGEL_REG_COLOR00) != 0x0f00) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!rigel_denise_get_video_desc(ctx, &video)) {
        rigel_destroy(ctx);
        return 1;
    }

    /* DIWSTRT=0x2c81 (hstrt=129, vstrt=44), DIWSTOP=0x2cc1 (hstop=193+256=449, vstop=44).
     * vstop==vstrt so height falls back to 256; width = 449-129 = 320. */
    if (video.display_width != 320 || video.display_height != 256) {
        rigel_destroy(ctx);
        return 1;
    }

    if (video.visible_x_start != 0x81 || video.visible_x_stop != 449) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, TEST_REG_BPLCON0, 0x9000u);

    if (!rigel_denise_get_video_desc(ctx, &video)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (video.display_width != 640 || video.display_height != 256 ||
        video.visible_x_start != 0x102 || video.visible_x_stop != 898) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, TEST_REG_BPLCON0, 0x0000u);

    rigel_custom_write16(ctx, 0x08e, 0x057e);
    rigel_custom_write16(ctx, 0x090, 0x40be);

    if (!rigel_denise_get_video_desc(ctx, &video)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (video.display_width != 320 || video.display_height != 256 ||
        video.visible_y_start != 0x05u || video.visible_y_stop != 0x105u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, 0x08e, 0x2c81);
    rigel_custom_write16(ctx, 0x090, 0x2cc1);

    if (!rigel_denise_get_video_desc(ctx, &video)) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_step(ctx, 4);

    if (!rigel_denise_get_debug_state(ctx, &debug)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (debug.scanline_counter == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if (debug.last_rgb32 == 0 || debug.visible_scanline) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!rigel_denise_get_current_scanline(ctx, &scanline)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (scanline.width != video.display_width || scanline.visible) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_step(ctx, frame_cycles);

    if (!rigel_denise_get_debug_state(ctx, &debug)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (debug.frame_counter != 1 || debug.beam_vpos != 0 || debug.beam_hpos != 4) {
        rigel_destroy(ctx);
        return 1;
    }

    {
        int rc = test_visual_bitplane_frame(ctx, chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_visual_bitplane_frame\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    {
        int rc = test_ddfstrt_alignment(ctx, chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_ddfstrt_alignment\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    {
        int rc = test_singlepf_bplcon1_scroll(ctx, chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_singlepf_bplcon1_scroll\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    {
        int rc = test_singlepf_bplcon1_even_plane_scroll(ctx, chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_singlepf_bplcon1_even_plane_scroll\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    {
        int rc = test_hires_prefetch_words_are_not_displayed(ctx, chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_hires_prefetch_words_are_not_displayed\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    {
        int rc = test_bitplane_dma_disable_stops_reuse(ctx, chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_bitplane_dma_disable_stops_reuse\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    {
        int rc = test_two_bitplane_fetch_window(ctx, chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_two_bitplane_fetch_window\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    {
        int rc = test_dualpf_bplcon1_independent_scroll(ctx, chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_dualpf_bplcon1_independent_scroll\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    {
        int rc = test_rgb565_frame_format(chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_rgb565_frame_format\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    {
        int rc = test_rgb565_write_target_edges(chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_rgb565_write_target_edges\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    {
        int rc = test_frame_metadata_flags(chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_frame_metadata_flags\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    {
        int rc = test_interlace_frame_flags(chip_ram);
        if (rc != 0) {
            fprintf(stderr, "FAILED: test_interlace_frame_flags\n");
            rigel_destroy(ctx);
            return 1;
        }
    }

    rigel_destroy(ctx);
    return 0;
}
