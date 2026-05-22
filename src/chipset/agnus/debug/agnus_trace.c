#include "agnus_trace.h"

#include <stdio.h>

void agnus_trace_init(agnus_trace_config_t *cfg)
{
    cfg->enabled  = false;
    cfg->copper   = false;
    cfg->blitter  = false;
    cfg->beam     = false;
    cfg->dma      = false;
}

void agnus_trace_enable(agnus_trace_config_t *cfg, bool on) { cfg->enabled = on; }

void agnus_trace_copper_move(const agnus_trace_config_t *cfg,
                             rigel_u16 addr, rigel_u16 val)
{
    if (!cfg->enabled || !cfg->copper) return;
    fprintf(stderr, "[copper] MOVE $%03X = $%04X\n", addr, val);
}

void agnus_trace_copper_wait(const agnus_trace_config_t *cfg,
                             rigel_u16 hpos, rigel_u16 vpos)
{
    if (!cfg->enabled || !cfg->copper) return;
    fprintf(stderr, "[copper] WAIT vpos=%d hpos=%d\n", vpos, hpos);
}

void agnus_trace_copper_skip(const agnus_trace_config_t *cfg,
                             rigel_u16 hpos, rigel_u16 vpos, bool taken)
{
    if (!cfg->enabled || !cfg->copper) return;
    fprintf(stderr, "[copper] SKIP vpos=%d hpos=%d %s\n",
            vpos, hpos, taken ? "taken" : "not taken");
}

void agnus_trace_blitter_start(const agnus_trace_config_t *cfg,
                               rigel_u32 width, rigel_u32 height)
{
    if (!cfg->enabled || !cfg->blitter) return;
    fprintf(stderr, "[blitter] start %ux%u words\n", width, height);
}

void agnus_trace_blitter_done(const agnus_trace_config_t *cfg, rigel_u32 cycles)
{
    if (!cfg->enabled || !cfg->blitter) return;
    fprintf(stderr, "[blitter] done in %u cycles\n", cycles);
}

void agnus_trace_beam(const agnus_trace_config_t *cfg,
                      rigel_u16 hpos, rigel_u16 vpos)
{
    if (!cfg->enabled || !cfg->beam) return;
    fprintf(stderr, "[beam] hpos=%d vpos=%d\n", hpos, vpos);
}
