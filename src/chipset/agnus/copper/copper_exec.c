#include "copper_exec.h"
#include "agnus/copper/copper.h"
#include "core/rigel_context.h"
#include "debug/log.h"
#include "mmio/custom_regs.h"

#if RIGEL_ENABLE_STDLIB_ENV
#include <stdio.h>
#include <stdlib.h>
#endif

static bool copper_dmacon_trace_enabled(void)
{
#if RIGEL_ENABLE_STDLIB_ENV
    static int enabled = -1;

    if (enabled < 0) {
        const char *env = getenv("RIGEL_COPPER_DMACON_TRACE");
        enabled = (env != NULL && env[0] != '\0' && env[0] != '0') ? 1 : 0;
    }

    return enabled != 0;
#else
    return false;
#endif
}

#if RIGEL_ENABLE_STDLIB_ENV
static bool video_probe_enabled(void)
{
    static int enabled = -1;

    if (enabled < 0) {
        const char *env = getenv("RIGEL_VIDEO_PROBE");
        enabled = (env != NULL && env[0] != '\0' && env[0] != '0') ? 1 : 0;
    }

    return enabled != 0;
}

static rigel_u32 video_probe_env_u32(const char *name, rigel_u32 fallback)
{
    const char *env = getenv(name);

    if (env == NULL || env[0] == '\0') {
        return fallback;
    }

    return (rigel_u32)strtoul(env, NULL, 0);
}

static bool video_probe_match(rigel_u32 frame, rigel_u16 vpos)
{
    static int initialized = 0;
    static rigel_u32 target_frame;
    static rigel_u32 vfrom;
    static rigel_u32 vto;

    if (!video_probe_enabled()) {
        return false;
    }

    if (!initialized) {
        target_frame = video_probe_env_u32("RIGEL_VIDEO_PROBE_FRAME", 0xffffffffu);
        vfrom = video_probe_env_u32("RIGEL_VIDEO_PROBE_VFROM", 0u);
        vto = video_probe_env_u32("RIGEL_VIDEO_PROBE_VTO", 0xffffffffu);
        initialized = 1;
    }

    if (target_frame != 0xffffffffu && frame != target_frame) {
        return false;
    }

    return (rigel_u32)vpos >= vfrom && (rigel_u32)vpos <= vto;
}
#endif

/* copper_exec_move: IR1 = register address (bits 8:1, bit 0 must be 0)
 *                   IR2 = 16-bit data value to write.
 * Respects COPCON CDANG: when clear, writes below 0x80 are blocked; when set,
 * writes below 0x40 are blocked. */
void copper_exec_move(RigelContext *ctx, rigel_u16 ir1, rigel_u16 ir2)
{
    rigel_u32 reg;
    bool cdang;

    if (ctx == NULL) return;

    reg   = (rigel_u32)(ir1 & 0x01FEu);
    cdang = (ctx->chipset.agnus.copper.copcon & 0x0002u) != 0;
    if ((!cdang && reg < 0x80u) || (cdang && reg < 0x40u)) {
        ctx->chipset.agnus.copper.waiting = false;
        ctx->chipset.agnus.copper.fetch_pending = false;
        ctx->chipset.agnus.copper.stopped_until_vbl = true;
        return;
    }

    ctx->chipset.denise.output.pending_flags |= (rigel_u32)RIGEL_FRAME_COPPER_ACTIVE;
    if (reg == 0x096u && copper_dmacon_trace_enabled()) {
#if RIGEL_ENABLE_STDLIB_ENV
        char msg[128];

        snprintf(msg, sizeof(msg),
                 "[RIGEL-COPPER-DMACON] value=%04x cop_pc=%06x h=%u v=%u frame=%llu",
                 (unsigned)ir2,
                 (unsigned)(ctx->chipset.agnus.copper.program_counter & 0x00ffffffu),
                 (unsigned)ctx->chipset.agnus.beam.hpos,
                 (unsigned)ctx->chipset.agnus.beam.vpos,
                 (unsigned long long)ctx->chipset.agnus.beam.frame_count);
        rigel_log_info(msg);
#endif
    }
    if (reg == 0x096u || reg == 0x100u ||
        (reg >= 0x0e0u && reg <= 0x0f6u) ||
        reg == 0x108u || reg == 0x10au ||
        (reg >= 0x180u && reg <= 0x19eu)) {
        static unsigned trace_count = 0u;
        bool trace_probe =
#if RIGEL_ENABLE_STDLIB_ENV
            video_probe_match((rigel_u32)(ctx->chipset.agnus.beam.frame_count & 0xffffffffu),
                              ctx->chipset.agnus.beam.vpos);
#else
            false;
#endif
        bool trace_legacy =
#if RIGEL_ENABLE_STDLIB_ENV
            !video_probe_enabled() && trace_count < 512u;
#else
            trace_count < 512u;
#endif
        if (trace_probe ||
            trace_legacy) {
            rigel_log_event_t event = {
                RIGEL_LOG_EVENT_COPPER_WRITE,
                "copper_write",
                {
                    reg,
                    ir2,
                    ctx->chipset.agnus.copper.program_counter & 0x00ffffffu,
                    ctx->chipset.agnus.beam.hpos,
                    ctx->chipset.agnus.beam.vpos,
                    (rigel_u32)(ctx->chipset.agnus.beam.frame_count & 0xffffffffu),
                    (rigel_u32)(ctx->chipset.agnus.beam.frame_count >> 32)
                },
                7u
            };
            rigel_log_event(&event);
            if (!trace_probe) {
                trace_count++;
            }
        }
    }
    custom_regs_write16(ctx, reg, ir2);
}

/* copper_exec_skip_test: returns true if the beam is already at or past the
 * target position encoded in (ir1, ir2), meaning the next instruction should
 * be skipped.
 *
 * SKIP format: IR1 bits[15:8] = VP, bits[7:1] = HP, bit 0 = 1
 *              IR2 bits[15:8] = VPM, bits[7:1] = HPM, bit 0 = 1 */
bool copper_exec_skip_test(rigel_u16 ir1, rigel_u16 ir2,
                           rigel_u16 hpos, rigel_u16 vpos)
{
    rigel_u16 tgt_vpos = (ir1 >> 8) & 0xFFu;
    rigel_u16 tgt_hpos =  ir1 & 0xFEu;
    rigel_u16 vpmask   = (ir2 >> 8) & 0xFFu;
    rigel_u16 hpmask   =  ir2 & 0xFEu;
    return copper_beam_cmp(vpos, hpos, tgt_vpos, tgt_hpos, vpmask, hpmask);
}
