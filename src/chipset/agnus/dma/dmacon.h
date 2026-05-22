#ifndef RIGEL_AGNUS_DMACON_H
#define RIGEL_AGNUS_DMACON_H

#include <stdbool.h>
#include "rigel/rigel_types.h"

/* Canonical DMACON bit definitions — single source of truth for the whole chipset.
 *
 * DMACON is a 15-bit register written via $DFF096 (SET bits when bit 15 = 1,
 * CLEAR bits when bit 15 = 0). Reading $DFF002 (DMACONR) returns the current value
 * plus the live BBUSY and BZERO bits from the blitter.
 *
 * Priority order (hardware): refresh → disk → audio0-3 → sprites → bitplanes → copper/blitter
 * All channels require DMAEN (master) to be active. */

/* --- Master enable -------------------------------------------------------- */
#define DMACON_DMAEN    0x0200u  /* bit 9 — master DMA enable; gates all channels */
#define DMACON_BLTPRI   0x0400u  /* bit 10 — blitter priority (nasty): steals CPU cycles */

/* --- Channel enables ------------------------------------------------------ */
#define DMACON_BPLEN    0x0100u  /* bit 8 — bitplane DMA                        */
#define DMACON_COPEN    0x0080u  /* bit 7 — copper DMA                          */
#define DMACON_BLTEN    0x0040u  /* bit 6 — blitter DMA                         */
#define DMACON_SPREN    0x0020u  /* bit 5 — sprite DMA                          */
#define DMACON_DSKEN    0x0010u  /* bit 4 — disk DMA                            */
#define DMACON_AUD3EN   0x0008u  /* bit 3 — audio channel 3 DMA                */
#define DMACON_AUD2EN   0x0004u  /* bit 2 — audio channel 2 DMA                */
#define DMACON_AUD1EN   0x0002u  /* bit 1 — audio channel 1 DMA                */
#define DMACON_AUD0EN   0x0001u  /* bit 0 — audio channel 0 DMA                */

/* Convenience mask: all four audio channels */
#define DMACON_AUDEN    (DMACON_AUD0EN | DMACON_AUD1EN | DMACON_AUD2EN | DMACON_AUD3EN)

/* --- Blitter status bits (read-only in DMACONR, not writable via DMACON) - */
#define DMACON_BBUSY    0x4000u  /* bit 14 — blitter busy (live)               */
#define DMACON_BZERO    0x2000u  /* bit 13 — blitter zero flag (live)          */

/* --- Write modifier bit -------------------------------------------------- */
#define DMACON_SETCLR   0x8000u  /* bit 15 — 1=SET bits in value, 0=CLEAR them */

/* --- Inline enable queries ----------------------------------------------- */

static inline bool dmacon_master(rigel_u16 d)   { return (d & DMACON_DMAEN) != 0; }
static inline bool dmacon_bplen(rigel_u16 d)    { return (d & (DMACON_DMAEN | DMACON_BPLEN))  == (DMACON_DMAEN | DMACON_BPLEN);  }
static inline bool dmacon_copen(rigel_u16 d)    { return (d & (DMACON_DMAEN | DMACON_COPEN))  == (DMACON_DMAEN | DMACON_COPEN);  }
static inline bool dmacon_blten(rigel_u16 d)    { return (d & (DMACON_DMAEN | DMACON_BLTEN))  == (DMACON_DMAEN | DMACON_BLTEN);  }
static inline bool dmacon_spren(rigel_u16 d)    { return (d & (DMACON_DMAEN | DMACON_SPREN))  == (DMACON_DMAEN | DMACON_SPREN);  }
static inline bool dmacon_dsken(rigel_u16 d)    { return (d & (DMACON_DMAEN | DMACON_DSKEN))  == (DMACON_DMAEN | DMACON_DSKEN);  }
static inline bool dmacon_auden(rigel_u16 d, unsigned ch) {
    static const rigel_u16 bits[4] = { DMACON_AUD0EN, DMACON_AUD1EN, DMACON_AUD2EN, DMACON_AUD3EN };
    if (ch > 3) return false;
    return (d & (DMACON_DMAEN | bits[ch])) == (DMACON_DMAEN | bits[ch]);
}

/* Apply a DMACON write (respects SETCLR bit). */
static inline rigel_u16 dmacon_apply_write(rigel_u16 current, rigel_u16 value) {
    if (value & DMACON_SETCLR)
        return current | (value & 0x1FFu);
    return current & ~(value & 0x1FFu);
}

#endif
