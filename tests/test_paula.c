#include "core/rigel_context.h"
#include "paula/paula_state.h"
#include "rigel/rigel.h"

static rigel_u8 g_test_adf[AMIGA_TRACK_SIZE];

static void test_disk_irq_raise(void *opaque, rigel_u16 mask)
{
    rigel_u16 *value = (rigel_u16 *)opaque;
    *value = (rigel_u16)(*value | mask);
}

typedef struct TestChipRam {
    rigel_u16 words[16];
} TestChipRam;

static void test_chip_ram_write16(void *opaque, rigel_u32 addr, rigel_u16 value)
{
    TestChipRam *ram = (TestChipRam *)opaque;
    rigel_u32 index = (addr >> 1) & 0x0fU;
    ram->words[index] = value;
}

int main(void)
{
    RigelPaula paula = { 0 };
    FloppyDrive drive = { 0 };
    paula_disk_irq_sink_t sink;
    rigel_u16 irq_mask = 0;
    TestChipRam ram = { 0 };
    rigel_chip_ram_if_t chip_ram = { 0 };
    rigel_config_t cfg = { 0 };
    RigelContext *ctx;

    rigel_paula_reset(&paula);

    if (paula.audio.channels != 4) {
        return 1;
    }

    if (paula.disk.drive != NULL) {
        return 1;
    }

    if (paula.serial.baud_divider != 0) {
        return 1;
    }

    if (paula.interrupts.intena != 0 || paula.interrupts.intreq != 0 || paula.interrupts.ipl != 0) {
        return 1;
    }

    rigel_paula_raise_irq(&paula, RIGEL_PAULA_INT_BLIT);
    if ((paula.interrupts.intreq & RIGEL_PAULA_INT_BLIT) == 0) {
        return 1;
    }

    rigel_paula_clear_irq(&paula, RIGEL_PAULA_INT_BLIT);
    if ((paula.interrupts.intreq & RIGEL_PAULA_INT_BLIT) != 0) {
        return 1;
    }

    sink.opaque = &irq_mask;
    sink.raise = test_disk_irq_raise;
    rigel_paula_set_disk_irq_sink(&paula, sink);

    floppy_init(&drive);
    floppy_insert(&drive, g_test_adf, sizeof(g_test_adf));

    chip_ram.opaque = &ram;
    chip_ram.write16 = test_chip_ram_write16;
    rigel_paula_set_disk_memory_if(&paula, chip_ram);
    rigel_paula_set_disk_drive(&paula, &drive);

    disk_write_dsklen(&paula.disk, RIGEL_PAULA_DSKLEN_DMAEN | 1u);
    disk_write_dsklen(&paula.disk, RIGEL_PAULA_DSKLEN_DMAEN | 1u);
    if (!paula.disk.dma_active) {
        return 1;
    }

    if (!disk_dma_wants_service(&paula.disk)) {
        return 1;
    }

    disk_dma_service_grant(&paula.disk);
    if (ram.words[0] != 0x4489u) {
        return 1;
    }

    if (disk_read_dskdatr(&paula.disk) != 0x4489u) {
        return 1;
    }

    if ((irq_mask & 0x0002u) == 0) {
        return 1;
    }

    rigel_paula_reset(&paula);
    rigel_paula_set_disk_irq_sink(&paula, sink);
    rigel_paula_set_disk_drive(&paula, NULL);
    disk_write_dsklen(&paula.disk, RIGEL_PAULA_DSKLEN_DMAEN | 1u);
    disk_write_dsklen(&paula.disk, RIGEL_PAULA_DSKLEN_DMAEN | 1u);

    if (disk_dma_wants_service(&paula.disk)) {
        return 1;
    }

    irq_mask = 0;
    disk_step(&paula.disk, RIGEL_PAULA_DISK_FAKE_DMA_CYCLES);
    if ((irq_mask & 0x0002u) == 0) {
        return 1;
    }

    cfg.chip_ram = chip_ram;
    ctx = rigel_create(&cfg);
    if (ctx == NULL) {
        return 1;
    }

    floppy_init(&drive);
    floppy_insert(&drive, g_test_adf, sizeof(g_test_adf));
    rigel_paula_set_disk_drive(&ctx->chipset.paula, &drive);

    rigel_custom_write16(ctx, RIGEL_REG_DSKPTH, 0x0000);
    rigel_custom_write16(ctx, RIGEL_REG_DSKPTL, 0x0000);
    rigel_custom_write16(ctx, RIGEL_REG_ADKCON, RIGEL_PAULA_ADKCON_SETCLR | RIGEL_PAULA_ADKCON_WORDSYNC);
    rigel_custom_write16(ctx, RIGEL_REG_DSKLEN, RIGEL_PAULA_DSKLEN_DMAEN | 1u);
    rigel_custom_write16(ctx, RIGEL_REG_DSKLEN, RIGEL_PAULA_DSKLEN_DMAEN | 1u);

    if ((rigel_custom_read16(ctx, RIGEL_REG_ADKCONR) & RIGEL_PAULA_ADKCON_WORDSYNC) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    if ((rigel_custom_read16(ctx, RIGEL_REG_DSKBYTR) & RIGEL_PAULA_DSKBYTR_DMAON) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_step(ctx, 1);

    if (rigel_custom_read16(ctx, RIGEL_REG_DSKDATR) != 0x4489u) {
        rigel_destroy(ctx);
        return 1;
    }

    if ((rigel_get_intreq(ctx) & 0x0002u) == 0) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);

    return 0;
}
