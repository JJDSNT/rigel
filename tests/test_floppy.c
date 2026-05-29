#include "rigel/rigel.h"
#include "paula/disk.h"

enum {
    TEST_CIA_REG_PRB  = 0x1,
    TEST_CIA_REG_DDRB = 0x3
};

static rigel_u8 g_test_adf[512u * 11u];

int main(void)
{
    rigel_config_t cfg = { 0 };
    RigelContext *ctx = rigel_create(&cfg);
    rigel_floppy_status_t status = { 0 };

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

    if (status.has_media || status.dma_active || !status.track0 || status.cylinder != 0 || status.side != 0) {
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
        !status.dma_active || !status.motor_on) {
        rigel_destroy(ctx);
        return 1;
    }

    if (!rigel_floppy_get_status(ctx, RIGEL_FLOPPY_DRIVE_DF0, &status) || status.dma_active) {
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
