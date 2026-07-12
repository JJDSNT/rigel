#ifndef RIGEL_BEAM_H
#define RIGEL_BEAM_H

#include <stdbool.h>

#include "rigel_types.h"

/* Scanning geometry plus the beam counters at a known chipset time.
 *
 * Captured with rigel_get_beam_geometry(); the beam position at a later
 * time can then be derived with rigel_beam_position_at() by pure
 * arithmetic, without stepping the chipset and without touching the
 * context — safe to evaluate from any host thread on a copied snapshot.
 *
 * The snapshot stays valid while the scan is uniform. It must be
 * recaptured after anything that rewrites the beam counters or the scan
 * geometry (VPOSW/VHPOSW-class strobes, BEAMCON0 and the ECS programmable
 * sync registers, video standard changes). */
typedef struct rigel_beam_geometry {
    rigel_cycle_t time;    /* chipset time the counters below describe    */
    rigel_u16 vpos;
    rigel_u16 hpos;        /* internal CCK counter; VHPOSR exposes hpos>>1 */
    rigel_u16 line_clocks; /* CCK per line, without the LOL extension     */
    rigel_u16 frame_lines; /* lines per frame, without the LOF extension  */
    rigel_u8  lof;         /* long frame: +1 line in the current frame    */
    rigel_u8  lol;         /* long line: +1 CCK in the current line       */
    rigel_u8  lof_toggle;  /* interlace: LOF flips at each frame boundary */
    rigel_u8  lol_toggle;  /* NTSC: LOL flips at each line boundary       */
    rigel_u16 vposr_high;  /* VPOSR & 0xFF00 (LOF | Agnus chip id)        */
} rigel_beam_geometry_t;

rigel_beam_geometry_t rigel_get_beam_geometry(const RigelContext *ctx);

/* Derive the beam counters at `time` (>= g->time) from a snapshot.
 *
 * Returns false when the scan is not uniform — interlace or NTSC long-line
 * toggling, or a transient LOF/LOL latched outside toggle mode — in which
 * case the caller must read the live VPOSR/VHPOSR instead. */
bool rigel_beam_position_at(const rigel_beam_geometry_t *g,
                            rigel_cycle_t time,
                            rigel_u16 *vpos,
                            rigel_u16 *hpos);

#endif
