# Backport Plan — temp/rigel_modified_files

Source: `temp/rigel_modified_files/` — a parallel fork of Rigel with a mix of
hardware fixes, API extensions, and larger architectural work.

Items are grouped by priority and risk. "KS3.1" marks items required (or strongly
helpful) for Kickstart 3.1 to boot.

---

## Priority 1 — Correct, isolated, implement immediately

### 1.1 DMACONR (0x002) read-back with live BBUSY/BZERO  *(KS3.1)*
**Why:** Virtually all software polls `dmaconr` (0x002) to wait for the blitter —
not `dmacon` (0x096). Today 0x002 falls through to the register file and returns
stale write-data or zero. KS3.1 startup and any blitter-using code hangs.

**What:** In `agnus_read.c`, add `AGNUS_DMACONR` case that ORs live
`blitter_is_busy()` (→ BBUSY bit 14) and `!result.zero` (→ BZERO bit 13) into the
base DMACON value. Expose `AGNUS_DMACONR` in `agnus_mmio_has_reg`.

**Files:** `agnus_regs.h`, `agnus_mmio.c`, `agnus_read.c`

---

### 1.2 INTENAR (0x01C) / INTREQR (0x01E) read routing  *(KS3.1)*
**Why:** These are the read-back addresses for INTENA/INTREQ. KS3.1 (and KS1.3)
reads 0x01C and 0x01E to inspect interrupt state; the write addresses (0x09A,
0x09C) are strobe-only. Today reads fall through.

**What:** `interrupt_domain_owns_reg` and `read_reg` cover `RIGEL_REG_INTENAR` /
`RIGEL_REG_INTREQR`. Add constants to `rigel_custom.h`.

**Files:** `rigel_custom.h`, `interrupt_domain.c`

---

### 1.3 `event_latched` for RIGEL_EVENT_COPPER  *(correctness)*
**Why:** Our current workaround preserves `triggered` across `copper_vbl_reload`.
This is fragile. The temp version uses a dedicated `event_latched` flag — set on
every MOVE execute, cleared at the top of each `rigel_step`. VBL reload never
touches it. Cleaner and race-free.

**What:**
- `copper_state_t`: add `bool event_latched`
- `copper_service.c`: set `event_latched = true` on MOVE execute
- `copper_domain.c`: set `event_latched = false` in `jump1`/`jump2`; set it in
  WAIT-release path too
- `rigel.c`: replace `copper_triggered_before` check with `event_latched`; clear it
  before `rigel_chipset_step`

**Files:** `copper.h`, `copper.c`, `copper_domain.c`, `copper_service.c`, `rigel.c`

---

### 1.4 `copper_vbl_reload()` — proper VBL restart  *(correctness + KS3.1)*
**Why:** Current `rigel_copper_domain_jump1` always starts a fetch. The VBL reload
should: (a) do nothing when `cop1lc == 0`, (b) only fire when DMAEN+COPEN are
both set.

**What:** New `copper_vbl_reload(copper_state_t *)` in `copper.c`/`.h`; wrapper
`rigel_copper_domain_vbl_reload` in `copper_domain.c`/`.h`. Called in
`slot_scheduler.c` at VERTB position, guarded on `dmacon & (DMAEN|COPEN)`.
Remove the `copper_was_triggered` preservation hack.

**Files:** `copper.h`, `copper.c`, `copper_domain.h`, `copper_domain.c`,
`slot_scheduler.c`

---

### 1.5 BEAMCON0 (0x1DC) write — PAL/NTSC selection  *(KS3.1)*
**Why:** KS3.1 writes BEAMCON0 early in startup (bit 5 = PAL). Without handling
this register, the beam geometry stays at the wrong default and video sync breaks.

**What:** Add `AGNUS_BEAMCON0 = 0x1DC` to `agnus_regs.h`. Handle it in
`agnus_write.c`: bit 5 sets `beam.frame_lines` to PAL (312) or NTSC (262) and the
matching `line_clocks`. Expose in `agnus_mmio_has_reg`.

**Files:** `agnus_regs.h`, `agnus_mmio.c`, `agnus_write.c`

---

### 1.6 `rigel_video_std_t` config field  *(KS3.1)*
**Why:** Host should be able to declare PAL at `rigel_create` time before the first
BEAMCON0 write arrives, so the beam starts with the right geometry.

**What:** Add `rigel_video_std_t` enum and `video_std` field to `rigel_config_t`.
In `rigel_create`, apply PAL geometry after `rigel_reset` if `video_std ==
RIGEL_VIDEO_PAL`.

**Files:** `rigel_config.h`, `rigel.c`

---

### 1.7 VHPOSR hpos in lores pixels (>> 1)  *(correctness)*
**Why:** Rigel counts `hpos` in CCKs (0–226). VHPOSR[7:0] is documented as lores
hpos (half CCK rate). Returning raw CCKs reports double the real position.

**What:** In `agnus_read.c` VHPOSR case: `hlow = (beam.hpos >> 1) & 0xFFu`.

**Files:** `agnus_read.c`

---

### 1.8 Log callback API  *(host integration)*
**Why:** Bare-metal or embedded hosts have no stderr. The current `rigel_log_info`
hardcodes `fprintf(stderr)`. A host-provided callback lets Rigel messages reach
any sink (serial port, kprintf, UI console).

**What:**
- `rigel_config.h`: `rigel_log_fn_t` typedef; `log_fn`/`log_opaque` in
  `rigel_config_t`
- `log.h`: declare `rigel_log_set_fn(rigel_log_fn_t, void *opaque)`
- `log.c`: global function pointer; `rigel_log_info` dispatches through it with
  stderr fallback
- `rigel.c`: call `rigel_log_set_fn(config->log_fn, config->log_opaque)` in
  `rigel_create`

**Files:** `rigel_config.h`, `debug/log.h`, `debug/log.c`, `rigel.c`

---

### 1.9 Serial `rigel_serial_set_tx_instant`  *(KS3.1 debug)*
**Why:** KS3.1 startup outputs serial debug text at 9600 baud. In simulation the
baud-rate wait loop burns thousands of cycles. `tx_instant` mode enqueues the byte
immediately and raises TBE, making serial output visible without timing loops.

**What:** `rigel_serial.h` declaration + `rigel_serial_api.c` implementation that
delegates to `serial_set_tx_instant` in the Paula serial domain.

**Files:** `rigel_serial.h`, `rigel_serial_api.c`

---

## Priority 2 — Correct, medium scope, implement after Priority 1

### 2.1 Slot scheduler: remove DIW vertical gating of bitplane DMA
**Why:** DIWSTRT/DIWSTOP control **Denise output**, not Agnus DMA. The scheduler
currently suppresses bitplane fetch outside the vertical display window — that is
wrong. Bitplane DMA runs on every non-VBL line when `BPLEN` and `depth > 0`.

**What:** Remove `vdiwstrt`/`vdiwstop` fields from `agnus_slot_scheduler_t`.
Remove `agnus_slot_scheduler_set_diw()`. Remove the `vpos >= vdiwstrt && vpos <=
vdiwstop` guard in `rebuild`. Add `depth` field driven by `set_depth()` called from
`denise.c` on BPLCON0 writes.

Remove the `agnus_slot_scheduler_set_diw` calls from `denise.c` (and the DIWSTRT/
DIWSTOP propagation path to the scheduler).

**Files:** `slot_scheduler.h`, `slot_scheduler.c`, `denise.c`

---

### 2.2 Bitplane modulo: pass values explicitly to `bplpt_apply_modulo`
**Why:** Storing `bplmod[]` in the `bitplane_pointers_t` struct means the modulo
value is captured at write time. If the host changes it mid-line the effect on the
wrong boundary. The new signature reads from the register file at apply-time.

**What:** Change signature to `bplpt_apply_modulo(p, nplanes, bpl1mod, bpl2mod)`.
Caller reads `AGNUS_BPLMOD1`/`AGNUS_BPLMOD2` from `rigel_context_read_reg` at
end-of-line. Remove `bplmod[]` array from struct. Remove `bplpt_set_modulo()`.

**Files:** `bitplane_pointers.h`, `bitplane_pointers.c`, `slot_scheduler.c`,
`agnus_write.c`

---

### 2.3 VPOSR: LOF bit and chip ID
**Why:** VPOSR bit 15 = LOF (long odd frame in interlace). Bit 14:8 = chip revision.
Today we return 0 for both. LOF is needed for interlace-aware software; chip ID
affects some compatibility checks in KS and tools.

**Note:** Chip ID value is a design choice. `0x20` = Fat Agnus (8372A). For a
plain OCS A500 target, `0x00` (8361/8370) is correct. Pick one and document it.

Depends on LOF/LOL beam fields (Priority 3).

**Files:** `agnus_read.c`, `beam.h`

---

## Priority 3 — Larger refactors, implement carefully

### 3.1 LOF/LOL beam fields
**Why:** Required for correct interlace timing (PAL/NTSC long frame). `beam_step`
rewritten as a tick-by-tick loop that respects LOF/LOL toggles.

**Risk:** Substantial rewrite of `beam_step`. Validate with `test_beam.c` additions
from temp.

**Files:** `beam.h`, `beam.c`

---

### 3.2 Copper IR1/IR2 encoding — align with HRM
**Why:** Current code has IR1=data, IR2=register (inverted from HRM). Temp corrects
to IR1=register, IR2=data. WAIT/SKIP detection moves from `ir2 & 1` to `ir1 & 1`.

**Risk:** Breaking change — all copper programs in tests need to swap word order.
`test_agnus_domains.c` and `test_copper.c` already updated in temp.

**Files:** `copper_service.c`, `copper_exec.c`, `copper_wait.c`, all tests with
inline copper programs.

---

### 3.3 Compositor absolute lores coordinate system + ddfstrt_lores
**Why:** Bitplane pixels and sprites should be placed at absolute lores positions
(0–1023), not relative to `visible_x_start`. This makes scroll and display window
interaction correct. Requires `ddfstrt_lores` forwarded from Agnus DDFSTRT write.

**Risk:** Large change to `compositor.c` and `display_window.c`. Requires updating
`test_denise.c` completely (temp has a full rewrite of that test).

**Files:** `compositor.c`, `display_window.c`, `denise_state.h`, `framebuffer.c`,
`agnus_write.c`

---

### 3.4 Blitter line mode rewrite (`BlitterLineState`)
**Why:** Current `blitter_line_step` uses ad-hoc fields (`line_d`). Temp introduces
a proper `BlitterLineState` struct with full incremental state, correct Bresenham
error term, and per-step result publishing. Also fixes `blitter_estimate_cycles`
for line mode (height_lines steps, not width×height).

**Files:** `blitter_types.h`, `blitter_line.h`, `blitter_line.c`, `blitter.h`,
`blitter_timing.c`, `blitter_dma.c`

---

## Not backporting

- **Removing DIWSTRT/DIWSTOP/BPLCON0-2/COLOR31 from `rigel_custom.h`**: Public API
  symbols. Moving them to internal enums is fine internally, but removing them from
  the public header breaks existing hosts. Keep them; add internal enums as aliases
  where needed.

- **Single framebuffer**: Removing the double buffer simplifies state but breaks
  concurrent-read correctness. Defer until the video output design is finalised.

- **Removing `raster.c` from CMakeLists**: Verify no remaining callers first.
  Removing it while `raster.h` is still included elsewhere will silently produce
  link errors.

---

## Kickstart 3.1 minimum requirements

To have KS3.1 reach the Workbench screen the following must be in place:

| # | Item | Priority above |
|---|------|----------------|
| 1 | INTENAR/INTREQR read-back | 1.2 |
| 2 | DMACONR live BBUSY/BZERO | 1.1 |
| 3 | BEAMCON0 PAL/NTSC | 1.5 |
| 4 | `rigel_video_std_t` PAL config | 1.6 |
| 5 | `event_latched` + proper VBL reload | 1.3 + 1.4 |
| 6 | Log callback (serial output visibility) | 1.8 |
| 7 | Serial `tx_instant` | 1.9 |
| 8 | DIW vertical gating removed | 2.1 |
| 9 | Bitplane modulo from register file | 2.2 |

Items 1–7 are the hard gate. Items 8–9 affect visual output but not boot.
