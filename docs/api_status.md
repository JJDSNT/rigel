# API Integration Status

_Last updated: 2026-05-26 — frame double-buffering, BPL1MOD/BPL2MOD, audio+disk deadline contributions, blitter LINE mode per-slot, raster config, refresh DMA, DIWSTRT/DIWSTOP cross-domain fix, BPLCON0/1/2 consolidated, Denise render unit tests (priority/HAM/dualpf/sprites), priority double-map fix_

Legend: ✅ done · ⚠️ partial / known issue · ❌ missing

---

## P1 — Bloqueia qualquer host real

### CIA + Teclado ✅

CIA-A e CIA-B estão no chipset. Step em E-clock (CCK/5) com acumulador de
resto para precisão. CIA-A TOD pulsado por VBL. IRQs roteados via Paula:
CIA-A → PORTS (IPL 2), CIA-B → EXTER (IPL 6).

```c
/* rigel_cia.h */
rigel_u8 rigel_cia_read(RigelContext *ctx, rigel_u32 cia_id, rigel_u8 reg);
void     rigel_cia_write(RigelContext *ctx, rigel_u32 cia_id, rigel_u8 reg, rigel_u8 value);

/* rigel_keyboard.h */
void rigel_keyboard_inject(RigelContext *ctx, rigel_u8 amiga_keycode, bool pressed);
```

**Notas de integração CIA:**
- CIA-A: endereços `0xBFExxx`, bytes ímpares. Reg = bits [11:8] do endereço.
- CIA-B: endereços `0xBFDxxx`, bytes pares. Idem.
- Timers CIA-A e CIA-B: programados via MMIO pelo CPU (não precisam de API própria).
- CIA-B TOD (INDEX pulse do floppy) ainda não incrementa — falta wiring do /INDEX.

---

### Serial — RX/TX ✅

```c
/* rigel_serial.h */
void rigel_serial_receive_byte(RigelContext *ctx, rigel_u8 byte);
bool rigel_serial_tx_available(const RigelContext *ctx);
bool rigel_serial_pop_tx_byte(RigelContext *ctx, rigel_u8 *byte_out);
```

TX FIFO de 16 bytes. IRQ RBF (recepção) e TBE (transmissão vazia) já funcionam.

---

## P2 — Problemas de polimento com impacto real

### RTC em `rigel_config_t` ✅

```c
typedef struct rigel_config {
    /* ... */
    rigel_rtc_model_t rtc_model; /* NONE = sem RTC */
    time_t            rtc_time;  /* 0 = usar clock do host */
} rigel_config_t;
```

RTC auto-avança com clock real do host. `rigel_rtc_set_model/time` continuam disponíveis para ajuste pós-init.

---

### `RIGEL_EVENT_AUDIO_READY` ✅

Dispara quando pelo menos um período de canal de áudio vence. O evento não depende
de mudança no valor mixado, então samples consecutivos idênticos ainda acordam o
host. O deadline de áudio considera canais recém-armados antes do primeiro step.

---

### Discos DF1–DF3 ✅

CIA-B PRB → floppy implementado. Writes a CIA-B PRB/DDRB agora:
- Fazem decode dos bits /SELx, /MTR, /STEP, DIR, SIDE
- Chamam `floppy_step()` para todos os 4 drives com os sinais correctos
- Actualizam `disk_state_t.drive` para o drive seleccionado (Paula DMA)
- Actualizam CIA-B PRA ext inputs com `/DSKRDY`, `/TRK0`, `/WPROT`, `/CHNG` do drive activo

**Nota:** CIA-B TOD (/INDEX pulse) ainda não incrementa — wiring de /INDEX pendente.

### Mouse e Joystick — fire button ✅ / movimento ✅ / botão direito ✅

O input está completo com CIA no chipset:

| Acção | API |
|---|---|
| Movimento (mouse / joy direcções) | `rigel_input_set_joydat(ctx, port, dat)` |
| Botão primário / LMB / fire | `rigel_input_set_fire(ctx, port, pressed)` → CIA-A PRA |
| Botão direito / second fire | `rigel_input_set_pot_button_x(ctx, port, pressed)` → POTGOR |
| Terceiro botão | `rigel_input_set_pot_button_y(ctx, port, pressed)` → POTGOR |

**Nota de movimento para mouse:** JOY0DAT/JOY1DAT são contadores de quadratura de
8 bits. O host gere a acumulação de deltas e chama `set_joydat` com o valor
actualizado: `((y & 0xFF) << 8) | (x & 0xFF)`.

---

## P3 — Qualidade da API

### `rigel_get_chipset()` removido da API pública ✅

Removido de `rigel.h`. Os testes acedem ao chipset via `#include "core/rigel_context.h"` e `&ctx->chipset` directamente (já incluído no build path de testes). Nenhum host externo usava este símbolo.

---

### `rigel_snapshot_t` não captura estado real ⚠️

Captura apenas `cycles`, `intreq`, `intena` — 3 campos de um estado com centenas.
Um host que implemente save/restore com esta API vai perder todo o estado do copper,
blitter, áudio, disco, beam.

**Acção:** marcar como `/* incomplete — do not use for save state */` no header,
ou expandir para capturar `RigelChipset` completo (trabalho significativo, deixar
para quando o estado interno estabilizar).

---

### FRAME_READY vs VBLANK semântica documentada ✅

`RIGEL_EVENT_FRAME_READY` dispara quando `frame_count` muda (beam wraps).
`RIGEL_EVENT_VBLANK` dispara quando `beam_in_vblank()` faz transição false→true.
Podem disparar no mesmo `rigel_step`. VBLANK é o sinal hardware (linhas 0–25);
FRAME_READY é o evento de "frame completo disponível". A relação está descrita
em `include/rigel/rigel_events.h` e no guia de integração.

---

### Frame buffer double-buffering ✅

`frame_rgba[2][MAX_LINES][MAX_PIXELS]` com `front_idx` atómico por frame.
Denise escreve sempre para `frame_rgba[1 ^ front_idx]`; no boundary de frame
`front_idx ^= 1`. Hosts single-thread e async lêem `frame_rgba[front_idx]`
sem race condition dentro do mesmo `rigel_step`.

---

## P4 — Funcionalidade futura planeada

| Item | Estado | Nota |
|---|---|---|
| `rigel_frame_t.flags` (interlace, HAM, etc.) | ✅ | HAM/DUAL_PF/SPRITES_ACTIVE; INTERLACE/COPPER reservados |
| `rigel_frame_t.delta` (dirty lines bitmask) | ✅ | `dirty_lines[5]` — 1 bit/linha; pending→completed no frame boundary |
| Pixel format config (`RGBA8888` / `RGB565` / `INDEXED_8BIT`) | ⚠️ | RGBA8888 default and RGB565 implemented, including external write target; INDEXED_8BIT pending |
| `rigel_get_scanline(ctx, y)` por linha arbitrária | ✅ | raster y 0-311; `pixels_rgba` → frame_rgba[y][visible_x_start] |
| Double-buffering de frame | ✅ | `frame_rgba[2]` + `front_idx`; swap atómico no boundary de frame |
| AUDIO_READY por-período com timestamp | ⚠️ | evento por período implementado; timestamp específico do sample ainda não |
| DF1–3 INDEX pulse para CIA-B TOD | ✅ | 300 RPM sintético via CCK counter |
| `rigel_snapshot_t` completo | ❌ | depende de estado interno estabilizar |
| Attached sprites (4bpp de pares) | ✅ | CTL bit7 → `denise_sprite_attached_pixel`; paleta 17–31 |
| BPLCON1 scroll offsets | ✅ | PF1H aplicado no compositor (shift de -s pixels) |
| BPLCON0 hires mode (extra 4 slots) | ✅ | scheduler usa passo 1 em hires (bit 15) |
| ECS feature gate + IDs | ✅ | `RIGEL_CHIPSET_ECS`; `VPOSR` Agnus ID; `DENISEID=$00fc` |
| ECS `BEAMCON0` mínimo | ✅ | bit 5 PAL/NTSC; outros bits mascarados até timing programável |
| ECS `DIWHIGH` | ✅ | expande `DIWSTRT/DIWSTOP` no modelo ECS |
| ECS Chip RAM window | ✅ | OCS wrap 512 KiB; ECS wrap 1 MiB; 2 MiB variants pendentes |
| ECS SuperHires/Productivity | ❌ | requer timing/render próprios |
| HAM6 mode | ✅ | `ham6_decode_pixel` ligado no compositor; estado prev_rgb por linha |
| EHB mode (Extra-Half-Brite) | ✅ | automático com 6 planos + !HAM + !DUAL; `ehb_resolve_color` |
| Dual-playfield mode | ✅ | `dualpf_decode` + BPLCON2 PF1P/PF2P resolve PF1 vs PF2 |
| Sprite/PF priority (BPLCON2) | ✅ | `pair + PFxP < 4` determina se sprite bate playfield; iteração 7→0 |

---

## Resumo de estado por subsistema

| Subsistema | Stepping | MMIO | IRQ | Host API | Notas |
|---|---|---|---|---|---|
| Beam / raster | ✅ | ✅ | — | ✅ | slot scheduler activo |
| Copper | ✅ | ✅ | ✅ | via events | MOVE/WAIT/SKIP completo |
| Blitter | ✅ | ✅ | ✅ | via events | BLTPRI + nasty; LINE mode per-slot via `blitter_line_step` |
| Bitplane DMA | ✅ | ✅ | — | `rigel_get_frame` | planar→chunky feito; BPL1MOD/BPL2MOD aplicados fim-de-linha |
| Audio | ✅ | ✅ | ✅ | `rigel_get_audio_sample` | AUDIO_READY por período; deadline `audio_cycles_to_next_event` wired |
| Disk / floppy | ✅ | ✅ | ✅ | insert/eject/status | DF0-3 CIA-B PRB wired ✅; deadline `disk_cycles_to_next_event` wired |
| Serial | ✅ | ✅ | ✅ | ✅ `rigel_serial_*` | TX FIFO 16 bytes, RBF+TBE IRQ |
| Input (joy/pot/mouse) | ✅ | ✅ | — | ✅ | joydat, fire(CIA-A), pot buttons |
| Teclado | ✅ via CIA-B SDR | ✅ | ✅ EXTER | `rigel_keyboard_inject` | ✅ |
| CIA-A / CIA-B | ✅ | ✅ step+MMIO | ✅ PORTS/EXTER | `rigel_cia_read/write` | CIA-B TOD sem /INDEX |
| RTC | ✅ | ✅ | — | ✅ | config + pós-init |
| Sprites | ✅ DMA | ✅ SPRxPOS/CTL/DATA/DATB | — | via Denise | overlay+BPLCON2 priority ✅; attached 4bpp ✅; CLXDAT/CLXCON ✅ |
| Denise video | ✅ | ✅ | — | `rigel_get_frame` / `rigel_get_scanline` / framebuffer target | HAM6/EHB/dual-pf/priority/attached ✅; dirty/flags ✅; RGBA8888/RGB565 |
| IRQ / IPL | ✅ | ✅ | ✅ | `rigel_get_ipl` | ✅ |
| Bus observation | ✅ | — | — | `rigel_get_bus_state` | slot-accurate |
