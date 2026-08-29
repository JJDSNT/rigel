#include "rigel/rigel.h"
#include "paula/disk.h"

enum {
    TEST_CIA_REG_PRA  = 0x0,
    TEST_CIA_REG_PRB  = 0x1,
    TEST_CIA_REG_DDRB = 0x3
};

static rigel_u8 g_test_adf[512u * 11u];

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    rigel_floppy_status_t status = { 0 };
    rigel_u32 drive_id = 0;
    int bit;
    int write;

    if (ctx == NULL) {
        return 1;
    }

    if (rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF0) ||
        rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF1) ||
        rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF2) ||
        rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF3)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!rigel_floppy_get_status(ctx, RIGEL_FLOPPY_DRIVE_DF0, &status)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (status.has_media || status.selected || status.dma_active || !status.track0 || status.cylinder != 0 || status.side != 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if (rigel_floppy_insert(ctx, RIGEL_FLOPPY_DRIVE_DF0, g_test_adf, sizeof(g_test_adf)) != RIGEL_STATUS_OK) {
        rigel_destroy(ctx);
        return 1;
    }

    if (rigel_floppy_insert(ctx, RIGEL_FLOPPY_DRIVE_DF1, g_test_adf, sizeof(g_test_adf)) != RIGEL_STATUS_OK ||
        rigel_floppy_insert(ctx, RIGEL_FLOPPY_DRIVE_DF2, g_test_adf, sizeof(g_test_adf)) != RIGEL_STATUS_OK ||
        rigel_floppy_insert(ctx, RIGEL_FLOPPY_DRIVE_DF3, g_test_adf, sizeof(g_test_adf)) != RIGEL_STATUS_OK) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF0)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF1) ||
        !rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF2) ||
        !rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF3)) {
        rigel_destroy(ctx);
        return 1;
    }

    /* A pending change in DF1 must not drive /CHNG while DF0 is selected. */
    rigel_cia_write(ctx, 1u, TEST_CIA_REG_DDRB, 0xffu);
    rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0xf6u); /* DF0, /STEP low */
    rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0xf7u); /* DF0, /STEP high */
    for (write = 0; write < 40; ++write) {
        /* Activity on DF0 must not consume DF1's drive-ID sequence. */
        rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0xf7u);
    }
    if ((rigel_cia_read(ctx, 0u, TEST_CIA_REG_PRA) & 0x04u) == 0u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0xefu); /* select DF1 */
    if ((rigel_cia_read(ctx, 0u, TEST_CIA_REG_PRA) & 0x04u) != 0u) {
        rigel_destroy(ctx);
        return 1;
    }
    /* Exercise the motor/select preamble and all 32 sampled ID bits. */
    rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0x6fu); /* motor on, DF1 */
    rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0x7fu); /* deselect */
    rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0xffu); /* motor off */
    rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0xefu); /* preamble select */
    rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0xffu); /* preamble deselect */
    for (bit = 0; bit < 32; ++bit) {
        rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0xefu);
        drive_id = (drive_id << 1u) |
            ((rigel_u32)(rigel_cia_read(ctx, 0u, TEST_CIA_REG_PRA) >> 5u) & 1u);
        rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0xffu);
    }
    if (drive_id != 0x00000000u) {
        /* A normal Amiga DD drive identifies as DRT_AMIGA. */
        rigel_destroy(ctx);
        return 1;
    }
    rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0xf7u); /* restore DF0 */

    if (!rigel_floppy_get_status(ctx, RIGEL_FLOPPY_DRIVE_DF2, &status) || !status.has_media || status.dma_active) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_DSKPTH, 0x0000);
    rigel_custom_write16(ctx, RIGEL_REG_DSKPTL, 0x0000);
    rigel_custom_write16(ctx, RIGEL_REG_DSKLEN, RIGEL_PAULA_DSKLEN_DMAEN | 1u);
    rigel_custom_write16(ctx, RIGEL_REG_DSKLEN, RIGEL_PAULA_DSKLEN_DMAEN | 1u);

    if (!rigel_floppy_get_status(ctx, RIGEL_FLOPPY_DRIVE_DF0, &status) || !status.dma_active) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_custom_write16(ctx, RIGEL_REG_DSKLEN, 0u);
    rigel_cia_write(ctx, 1u, TEST_CIA_REG_DDRB, 0xffu);
    rigel_cia_write(ctx, 1u, TEST_CIA_REG_PRB, 0x5fu); /* /MTR=0, /SEL2=0 */
    rigel_custom_write16(ctx, RIGEL_REG_DSKPTH, 0x0000);
    rigel_custom_write16(ctx, RIGEL_REG_DSKPTL, 0x0000);
    rigel_custom_write16(ctx, RIGEL_REG_DSKLEN, RIGEL_PAULA_DSKLEN_DMAEN | 1u);
    rigel_custom_write16(ctx, RIGEL_REG_DSKLEN, RIGEL_PAULA_DSKLEN_DMAEN | 1u);

    if (!rigel_floppy_get_status(ctx, RIGEL_FLOPPY_DRIVE_DF2, &status) ||
        !status.dma_active || !status.motor_on || !status.selected) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!rigel_floppy_get_status(ctx, RIGEL_FLOPPY_DRIVE_DF0, &status) ||
        status.dma_active || status.selected) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_reset(ctx);
    if (!rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF0)) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF1) ||
        !rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF2) ||
        !rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF3)) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_floppy_eject(ctx, RIGEL_FLOPPY_DRIVE_DF0);
    if (rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF0)) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_floppy_eject(ctx, RIGEL_FLOPPY_DRIVE_DF1);
    rigel_floppy_eject(ctx, RIGEL_FLOPPY_DRIVE_DF2);
    rigel_floppy_eject(ctx, RIGEL_FLOPPY_DRIVE_DF3);

    if (rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF1) ||
        rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF2) ||
        rigel_floppy_has_media(ctx, RIGEL_FLOPPY_DRIVE_DF3)) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
