#include "rigel/rigel.h"

#include "agnus/agnus_state.h"
#include "agnus/blitter/blitter.h"
#include "cia/cia.h"
#include "domains/interrupt/interrupt_domain.h"

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    BlitterIrqSink sink;
    BlitterState blitter;

    if (ctx == NULL) {
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_INTENA, 0xc001);
    rigel_custom_write16(ctx, RIGEL_REG_INTREQ, 0x8001);

    if (rigel_get_intena(ctx) != 0x4001 || rigel_get_intreq(ctx) != 0x0001) {
        rigel_destroy(ctx);
        return 1;
    }

    if (rigel_get_ipl(ctx) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_reset(ctx);
    sink = rigel_agnus_blitter_irq_sink(ctx);
    blitter_init(&blitter);
    blitter_force_finish(&blitter, sink);

    if ((rigel_get_intreq(ctx) & 0x0040u) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, AGNUS_BLTSIZE, 0x0001u);
    if ((rigel_get_intreq(ctx) & 0x0040u) != 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_reset(ctx);
    rigel_custom_write16(ctx, RIGEL_REG_INTENA, 0x8008u);
    rigel_cia_write(ctx, 0u, CIA_REG_ICR, CIA_ICR_SETCLR | CIA_ICR_SP);
    rigel_cia_write(ctx, 1u, CIA_REG_ICR, CIA_ICR_SETCLR | CIA_ICR_SP);
    rigel_keyboard_inject(ctx, 0x20u, true);

    if ((rigel_get_intreq(ctx) & RIGEL_PAULA_INT_PORTS) == 0u ||
        (rigel_get_intreq(ctx) & RIGEL_PAULA_INT_EXTER) != 0u) {
        rigel_destroy(ctx);
        return 1;
    }
    if ((rigel_cia_read(ctx, 0u, CIA_REG_ICR) & CIA_ICR_SP) == 0u ||
        (rigel_cia_read(ctx, 1u, CIA_REG_ICR) & CIA_ICR_SP) != 0u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
