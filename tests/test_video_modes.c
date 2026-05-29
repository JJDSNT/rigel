#include "rigel/rigel.h"
#include "core/rigel_context.h"

enum {
    BPLCON0_HIRES = 0x8000u,
    BPLCON0_HAM   = 0x0800u,
    BPLCON0_DUAL  = 0x0400u,
    BPLCON0_LACE  = 0x0004u,
    BPLCON2_KILLEHB = 0x0040u
};

static int make_context(rigel_video_std_t std, rigel_chipset_model_t model, RigelContext **out)
{
    rigel_config_t cfg = { 0 };

    cfg.video_std = std;
    cfg.chipset_model = model;
    *out = rigel_create(&cfg);
    return *out == 0;
}

static int check_mode(
    RigelContext *ctx,
    rigel_u16 bplcon0,
    rigel_u16 bplcon2,
    rigel_u16 expected_flags
)
{
    rigel_denise_debug_state_t debug;

    rigel_custom_write16(ctx, RIGEL_REG_BPLCON2, bplcon2);
    rigel_custom_write16(ctx, RIGEL_REG_BPLCON0, bplcon0);

    if (!rigel_denise_get_debug_state(ctx, &debug)) {
        return 1;
    }

    if (debug.active_mode_flags != expected_flags) {
        return 1;
    }

    if (((expected_flags & RIGEL_DENISE_MODE_LACE) != 0u) !=
        (ctx->chipset.agnus.beam.lof_toggle != 0u)) {
        return 1;
    }

    return 0;
}

static int check_standard_modes(void)
{
    static const rigel_video_std_t standards[] = {
        RIGEL_VIDEO_NTSC,
        RIGEL_VIDEO_PAL
    };
    unsigned s;

    for (s = 0; s < sizeof(standards) / sizeof(standards[0]); ++s) {
        RigelContext *ctx;
        rigel_u32 expected_frame_cycles;

        if (make_context(standards[s], RIGEL_CHIPSET_OCS, &ctx)) {
            return 1;
        }

        expected_frame_cycles = 227u * (standards[s] == RIGEL_VIDEO_PAL ? 312u : 262u);
        if (rigel_get_frame_cycles(ctx) != expected_frame_cycles) {
            rigel_destroy(ctx);
            return 1;
        }

        if (check_mode(ctx, 1u << 12, 0u, 0u) ||
            check_mode(ctx, (1u << 12) | BPLCON0_HIRES, 0u, RIGEL_DENISE_MODE_HIRES) ||
            check_mode(ctx, (1u << 12) | BPLCON0_LACE, 0u, RIGEL_DENISE_MODE_LACE) ||
            check_mode(ctx, (1u << 12) | BPLCON0_HIRES | BPLCON0_LACE, 0u,
                       RIGEL_DENISE_MODE_HIRES | RIGEL_DENISE_MODE_LACE)) {
            rigel_destroy(ctx);
            return 1;
        }

        rigel_destroy(ctx);
    }

    return 0;
}

static int check_special_ocs_modes(void)
{
    static const rigel_video_std_t standards[] = {
        RIGEL_VIDEO_NTSC,
        RIGEL_VIDEO_PAL
    };
    unsigned s;

    for (s = 0; s < sizeof(standards) / sizeof(standards[0]); ++s) {
        RigelContext *ctx;

        if (make_context(standards[s], RIGEL_CHIPSET_OCS, &ctx)) {
            return 1;
        }

        if (check_mode(ctx, (6u << 12), 0u, RIGEL_DENISE_MODE_EHB) ||
            check_mode(ctx, (6u << 12) | BPLCON0_LACE, 0u,
                       RIGEL_DENISE_MODE_EHB | RIGEL_DENISE_MODE_LACE) ||
            check_mode(ctx, (6u << 12) | BPLCON0_HAM, 0u, RIGEL_DENISE_MODE_HAM) ||
            check_mode(ctx, (6u << 12) | BPLCON0_HAM | BPLCON0_LACE, 0u,
                       RIGEL_DENISE_MODE_HAM | RIGEL_DENISE_MODE_LACE)) {
            rigel_destroy(ctx);
            return 1;
        }

        rigel_destroy(ctx);
    }

    return 0;
}

static int check_ecs_surface(void)
{
    RigelContext *ctx;
    rigel_denise_debug_state_t debug;

    if (make_context(RIGEL_VIDEO_NTSC, RIGEL_CHIPSET_ECS, &ctx)) {
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_BEAMCON0, 0x0020u);
    if (rigel_get_frame_cycles(ctx) != 227u * 312u) {
        rigel_destroy(ctx);
        return 1;
    }

    if (check_mode(ctx, (6u << 12), BPLCON2_KILLEHB, 0u)) {
        rigel_destroy(ctx);
        return 1;
    }
    rigel_custom_write16(ctx, RIGEL_REG_BPLCON0, (6u << 12));
    rigel_custom_write16(ctx, RIGEL_REG_BPLCON2, BPLCON2_KILLEHB);
    if (!rigel_denise_get_debug_state(ctx, &debug) ||
        (debug.active_mode_flags & RIGEL_DENISE_MODE_EHB) != 0u) {
        rigel_destroy(ctx);
        return 1;
    }

    /*
     * ECS-only SuperHires/Productivity/Euro/Dbl modes are not implemented yet.
     * Keep this guard so unsupported modes do not appear as public mode flags.
     */
    if (check_mode(ctx, (1u << 12) | 0x0080u, 0u, 0u)) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}

int main(void)
{
    if (check_standard_modes() ||
        check_special_ocs_modes() ||
        check_ecs_surface()) {
        return 1;
    }

    return 0;
}
