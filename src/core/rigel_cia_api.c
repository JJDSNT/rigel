#include "rigel/rigel_cia.h"
#include "rigel/rigel_keyboard.h"

#include "cia/cia.h"
#include "cia/cia_ports.h"
#include "chipset/chipset.h"
#include "core/rigel_context.h"
#include "floppy/floppy_drive.h"
#include "paula/paula_state.h"

/* CIA-B PRB → floppy drive routing.
 * Called after any CIA-B write that may affect drive control signals. */
static void cia_b_prb_update_floppy(RigelContext *ctx)
{
    CIA *cia_b;
    CIA *cia_a;
    uint8_t prb;
    uint8_t pra;
    FloppySignals sig;
    int sel[4];
    int mtr, i, idmode, selected_count;
    FloppyDrive *active;

    cia_b = &ctx->chipset.cia[1];
    cia_a = &ctx->chipset.cia[0];
    prb   = cia_port_b_value(cia_b);

    /* Decode CIA-B PRB (active-low drive control signals) */
    mtr    = !(prb & 0x80u);        /* bit 7: /MTR  */
    sel[3] = !(prb & 0x40u);        /* bit 6: /SEL3 */
    sel[2] = !(prb & 0x20u);        /* bit 5: /SEL2 */
    sel[1] = !(prb & 0x10u);        /* bit 4: /SEL1 */
    sel[0] = !(prb & 0x08u);        /* bit 3: /SEL0 */
    sig.side      = (int)(((prb >> 2) & 1u) == 0u); /* bit 2: /SIDE active-low, 0=side1, 1=side0 */
    sig.direction = (int)((prb >> 1) & 1u); /* bit 1: DIR  (1=out) */
    sig.step      = !(prb & 0x01u);         /* bit 0: /STEP active-low */

    for (i = 0; i < 4; i++) {
        sig.selected = sel[i];
        sig.motor    = sel[i] ? mtr : 0;
        floppy_step(&ctx->chipset.floppy[i], &sig);
    }

    /* Route DMA reads to first selected drive (hardware allows only one at a time) */
    active = &ctx->chipset.floppy[0];
    selected_count = 0;
    for (i = 0; i < 4; i++) {
        if (sel[i]) {
            if (selected_count == 0) {
                active = &ctx->chipset.floppy[i];
            }
            selected_count++;
        }
    }
    rigel_paula_set_disk_drive(&ctx->chipset.paula, active);

    /*
     * Update CIA-A PRA ext inputs (bits 2-5) with drive status.
     * Read-modify-write preserves fire button bits (6-7) set elsewhere.
     * CIA-A PRA: bit2=/CHNG, bit3=/WPROT, bit4=/TRK0, bit5=/DSKRDY (active low).
     */
    pra = cia_port_a_value(cia_a);
    pra |= 0x3Cu; /* default all floppy lines high (inactive) */

    if (selected_count == 1) {
        idmode = !mtr && (active->id_count < 32);
        if (idmode) {
            /* During drive ID scan, /CHNG outputs the ID shift register bit */
            if (!floppy_get_idbit(active))
                pra &= ~0x04u;  /* bit 2: /CHNG */
        } else {
            if (!floppy_get_dskchg(active, active->motor))
                pra &= ~0x04u;  /* bit 2: /CHNG active low (0 = change sensed) */
        }
        if (!floppy_get_wpro(active))
            pra &= ~0x08u;  /* bit 3: /WPROT active low */
        if (floppy_get_track0(active))
            pra &= ~0x10u;  /* bit 4: /TRK0 active low */
        if (floppy_get_ready(active))
            pra &= ~0x20u;  /* bit 5: /DSKRDY active low */
    }

    cia_set_external_pra(cia_a, pra);
}

rigel_u8 rigel_cia_read(RigelContext *ctx, rigel_u32 cia_id, rigel_u8 reg)
{
    if (ctx == NULL || cia_id >= RIGEL_CIA_COUNT) {
        return 0xFFu;
    }

    return cia_read_reg(&ctx->chipset.cia[cia_id], reg);
}

void rigel_cia_write(RigelContext *ctx, rigel_u32 cia_id, rigel_u8 reg, rigel_u8 value)
{
    if (ctx == NULL || cia_id >= RIGEL_CIA_COUNT) {
        return;
    }

    cia_write_reg(&ctx->chipset.cia[cia_id], reg, value);

    /* Route CIA-B PRB/DDRB writes to floppy drive control logic */
    if (cia_id == 1 && (reg == CIA_REG_PRB || reg == CIA_REG_DDRB)) {
        cia_b_prb_update_floppy(ctx);
    }
}

void rigel_keyboard_inject(RigelContext *ctx, rigel_u8 amiga_keycode, bool pressed)
{
    rigel_u8 sdr_byte;

    if (ctx == NULL) {
        return;
    }

    /*
     * Amiga keyboard wire format: ~((keycode << 1) | up_flag), MSB first.
     * up_flag = 0 for keydown, 1 for keyup.
     * cia_receive_sdr injects the byte into CIA-A SDR and fires SP interrupt
     * -> Paula INTREQ PORTS -> IPL 2.
     */
    sdr_byte = (rigel_u8)(~(((rigel_u8)(amiga_keycode << 1u)) | (pressed ? 0u : 1u)));
    cia_receive_sdr(&ctx->chipset.cia[0], sdr_byte);
}
