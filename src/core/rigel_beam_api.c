#include "rigel/rigel_beam.h"

#include <stddef.h>

#include "chipset/agnus/mmio/agnus_mmio.h"
#include "core/rigel_context.h"

rigel_beam_geometry_t rigel_get_beam_geometry(const RigelContext *ctx)
{
    rigel_beam_geometry_t geometry = { 0 };

    if (ctx == NULL) {
        return geometry;
    }

    geometry.time = ctx->chipset.cycles;
    geometry.vpos = ctx->chipset.agnus.beam.vpos;
    geometry.hpos = ctx->chipset.agnus.beam.hpos;
    geometry.line_clocks = ctx->chipset.agnus.beam.line_clocks;
    geometry.frame_lines = ctx->chipset.agnus.beam.frame_lines;
    geometry.lof = ctx->chipset.agnus.beam.lof;
    geometry.lol = ctx->chipset.agnus.beam.lol;
    geometry.lof_toggle = ctx->chipset.agnus.beam.lof_toggle;
    geometry.lol_toggle = ctx->chipset.agnus.beam.lol_toggle;
    geometry.vposr_high = (rigel_u16)(
        ((rigel_u16)geometry.lof << 15) |
        (agnus_vposr_chip_id(&ctx->chipset.agnus) << 8)
    );
    return geometry;
}

bool rigel_beam_position_at(const rigel_beam_geometry_t *geometry,
                            rigel_cycle_t time,
                            rigel_u16 *vpos,
                            rigel_u16 *hpos)
{
    rigel_cycle_t elapsed;
    rigel_cycle_t clocks;
    rigel_cycle_t lines;

    if (geometry == NULL || vpos == NULL || hpos == NULL ||
        time < geometry->time || geometry->line_clocks == 0u ||
        geometry->frame_lines == 0u ||
        geometry->hpos >= geometry->line_clocks ||
        geometry->vpos >= geometry->frame_lines) {
        return false;
    }

    /* LOF/LOL make the frame or line length state-dependent.  Until the
     * public snapshot carries enough phase information to reproduce their
     * transitions exactly, reject them and let the host use the live path. */
    if (geometry->lof != 0u || geometry->lol != 0u ||
        geometry->lof_toggle != 0u || geometry->lol_toggle != 0u) {
        return false;
    }

    elapsed = time - geometry->time;
    clocks = (rigel_cycle_t)geometry->hpos + elapsed;
    lines = clocks / geometry->line_clocks;

    *hpos = (rigel_u16)(clocks % geometry->line_clocks);
    *vpos = (rigel_u16)(((rigel_cycle_t)geometry->vpos + lines) %
                       geometry->frame_lines);
    return true;
}
