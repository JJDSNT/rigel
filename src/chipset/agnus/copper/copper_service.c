#include "agnus/copper/copper_service.h"

#include "agnus/agnus_state.h"
#include "agnus/copper/copper_exec.h"
#include "agnus/copper/copper_wait.h"
#include "core/rigel_context.h"
#include "debug/log.h"
#include "domains/copper/copper_domain.h"
#include "mmio/custom_regs.h"

#if RIGEL_ENABLE_STDLIB_ENV
#include <stdio.h>
#include <stdlib.h>
#endif

static bool rigel_copper_trace_enabled(void)
{
#if RIGEL_ENABLE_STDLIB_ENV
    static int enabled = -1;

    if (enabled < 0) {
        const char *env = getenv("RIGEL_COPPER_TRACE");
        enabled = (env != NULL && env[0] != '\0' && env[0] != '0') ? 1 : 0;
    }

    return enabled != 0;
#else
    return false;
#endif
}

static rigel_u32 rigel_copper_trace_frame_from(void)
{
#if RIGEL_ENABLE_STDLIB_ENV
    static int initialized = 0;
    static rigel_u32 frame_from = 0u;

    if (!initialized) {
        const char *env = getenv("RIGEL_COPPER_TRACE_FRAME_FROM");
        if (env != NULL && env[0] != '\0') {
            frame_from = (rigel_u32)strtoul(env, NULL, 0);
        }
        initialized = 1;
    }

    return frame_from;
#else
    return 0u;
#endif
}

static bool rigel_copper_trace_pc(rigel_u32 pc)
{
    rigel_u32 chip_pc = pc & 0x001ffffeu;

    return chip_pc < 0x200000u;
}

static void rigel_copper_trace_fetch(RigelContext *ctx, const copper_state_t *copper,
                                     rigel_u16 ir1, rigel_u16 ir2)
{
#if RIGEL_ENABLE_STDLIB_ENV
    static unsigned count = 0u;
    char msg[192];

    if (ctx == NULL || copper == NULL || !rigel_copper_trace_enabled()) {
        return;
    }
    if (ctx->chipset.agnus.beam.frame_count < rigel_copper_trace_frame_from()) {
        return;
    }
    if (count >= 4096u || !rigel_copper_trace_pc(copper->program_counter)) {
        return;
    }

    snprintf(msg, sizeof(msg),
             "[RIGEL-COPPER-FETCH] pc=%06x ir=%04x:%04x h=%u v=%u frame=%llu "
             "wait=%d wb=%d stop=%d fp=%d cop1=%06x cop2=%06x dmacon=%04x",
             (unsigned)(copper->program_counter & 0x001ffffeu),
             (unsigned)ir1,
             (unsigned)ir2,
             (unsigned)ctx->chipset.agnus.beam.hpos,
             (unsigned)ctx->chipset.agnus.beam.vpos,
             (unsigned long long)ctx->chipset.agnus.beam.frame_count,
             copper->waiting ? 1 : 0,
             copper->wait_blitter ? 1 : 0,
             copper->stopped_until_vbl ? 1 : 0,
             copper->fetch_pending ? 1 : 0,
             (unsigned)(copper->cop1lc & 0x001ffffeu),
             (unsigned)(copper->cop2lc & 0x001ffffeu),
             (unsigned)(ctx->chipset.agnus.dma.dmacon & 0x7ffu));
    rigel_log_info(msg);
    count++;
#else
    (void)ctx;
    (void)copper;
    (void)ir1;
    (void)ir2;
#endif
}

static rigel_u16 rigel_agnus_copper_fetch16(RigelContext *ctx, rigel_u32 addr)
{
    rigel_chip_ram_if_t mem;

    if (ctx == NULL) {
        return 0;
    }

    mem = rigel_context_chip_ram(ctx);
    if (mem.read16 == NULL) {
        return 0;
    }

    return mem.read16(mem.opaque, addr);
}

void rigel_copper_service_step_program(RigelContext *ctx)
{
    RigelAgnus *agnus;
    copper_state_t *copper;
    rigel_u32 pc_before;
    rigel_u16 ir1;
    rigel_u16 ir2;

    if (ctx == NULL) {
        return;
    }

    agnus = &ctx->chipset.agnus;
    copper = &agnus->copper;
    if (copper->stopped_until_vbl) {
        return;
    }
    if (!copper->enabled || !copper->fetch_pending) {
        return;
    }

    ir1 = rigel_agnus_copper_fetch16(ctx, copper->program_counter);
    ir2 = rigel_agnus_copper_fetch16(ctx, copper->program_counter + 2u);
    rigel_copper_trace_fetch(ctx, copper, ir1, ir2);
    copper->fetch_pending = false;

    if ((ir1 & 0x0001u) != 0) {
        /* WAIT or SKIP — IR1 bit 0 = 1 distinguishes from MOVE */
        if (ir2 & 0x0001u) {
            /* SKIP: advance past the next instruction if beam condition is met */
            bool skip = copper_exec_skip_test(ir1, ir2,
                                              agnus->beam.hpos, agnus->beam.vpos);
            copper->program_counter += skip ? 8u : 4u;
            copper->fetch_pending = true;
        } else {
            /* WAIT: stall until beam reaches the target position */
            copper_wait_arm(copper, ir1, ir2);
            /* fetch_pending stays false — copper_domain_step releases the wait */
        }
        return;
    }

    /* MOVE: IR1 = register address (even, bit 0 = 0), IR2 = data */
    pc_before = copper->program_counter;
    copper_exec_move(ctx, ir1, ir2);
    if (copper->stopped_until_vbl) {
        return;
    }
    if (copper->program_counter == pc_before) {
        copper->program_counter += 4u;
    }
    copper->triggered     = true;
    copper->event_latched = true;
    copper->fetch_pending = true;
}
