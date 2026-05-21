#include "paula/paula_state.h"
#include "riegel/riegel.h"

static void test_disk_irq_raise(void *opaque, riegel_u16 mask)
{
    riegel_u16 *value = (riegel_u16 *)opaque;
    *value = (riegel_u16)(*value | mask);
}

typedef struct TestChipRam {
    riegel_u16 words[16];
} TestChipRam;

static void test_chip_ram_write16(void *opaque, riegel_u32 addr, riegel_u16 value)
{
    TestChipRam *ram = (TestChipRam *)opaque;
    riegel_u32 index = (addr >> 1) & 0x0fU;
    ram->words[index] = value;
}

int main(void)
{
    RiegelPaula paula;
    paula_disk_irq_sink_t sink;
    riegel_u16 irq_mask = 0;
    TestChipRam ram = { 0 };
    riegel_chip_ram_if_t chip_ram = { 0 };
    riegel_config_t cfg = { 0 };
    RiegelContext *ctx;

    riegel_paula_reset(&paula);

    if (paula.audio.channels != 4) {
        return 1;
    }

    if (paula.disk.inserted != 0) {
        return 1;
    }

    if (paula.serial.baud_divider != 0) {
        return 1;
    }

    if (paula.interrupts.intena != 0 || paula.interrupts.intreq != 0 || paula.interrupts.ipl != 0) {
        return 1;
    }

    riegel_paula_raise_irq(&paula, RIEGEL_PAULA_INT_BLIT);
    if ((paula.interrupts.intreq & RIEGEL_PAULA_INT_BLIT) == 0) {
        return 1;
    }

    riegel_paula_clear_irq(&paula, RIEGEL_PAULA_INT_BLIT);
    if ((paula.interrupts.intreq & RIEGEL_PAULA_INT_BLIT) != 0) {
        return 1;
    }

    sink.opaque = &irq_mask;
    sink.raise = test_disk_irq_raise;
    riegel_paula_set_disk_irq_sink(&paula, sink);

    chip_ram.opaque = &ram;
    chip_ram.write16 = test_chip_ram_write16;
    riegel_paula_set_disk_memory_if(&paula, chip_ram);
    riegel_paula_set_disk_inserted(&paula, 1);

    disk_write_dsklen(&paula.disk, RIEGEL_PAULA_DSKLEN_DMAEN | 1u);
    disk_write_dsklen(&paula.disk, RIEGEL_PAULA_DSKLEN_DMAEN | 1u);
    if (!paula.disk.dma_active) {
        return 1;
    }

    if (!disk_dma_wants_service(&paula.disk)) {
        return 1;
    }

    disk_dma_service_grant(&paula.disk);
    if (ram.words[0] != 0) {
        return 1;
    }

    if ((irq_mask & 0x0002u) == 0) {
        return 1;
    }

    riegel_paula_reset(&paula);
    riegel_paula_set_disk_irq_sink(&paula, sink);
    disk_write_dsklen(&paula.disk, RIEGEL_PAULA_DSKLEN_DMAEN | 1u);
    disk_write_dsklen(&paula.disk, RIEGEL_PAULA_DSKLEN_DMAEN | 1u);

    if (disk_dma_wants_service(&paula.disk)) {
        return 1;
    }

    irq_mask = 0;
    disk_step(&paula.disk, RIEGEL_PAULA_DISK_FAKE_DMA_CYCLES);
    if ((irq_mask & 0x0002u) == 0) {
        return 1;
    }

    cfg.chip_ram = chip_ram;
    ctx = riegel_create(&cfg);
    if (ctx == NULL) {
        return 1;
    }

    riegel_custom_write16(ctx, RIEGEL_REG_DSKPTH, 0x0000);
    riegel_custom_write16(ctx, RIEGEL_REG_DSKPTL, 0x0000);
    riegel_custom_write16(ctx, RIEGEL_REG_ADKCON, RIEGEL_PAULA_ADKCON_SETCLR | RIEGEL_PAULA_ADKCON_WORDSYNC);
    riegel_custom_write16(ctx, RIEGEL_REG_DSKLEN, RIEGEL_PAULA_DSKLEN_DMAEN | 1u);
    riegel_custom_write16(ctx, RIEGEL_REG_DSKLEN, RIEGEL_PAULA_DSKLEN_DMAEN | 1u);

    if ((riegel_custom_read16(ctx, RIEGEL_REG_ADKCONR) & RIEGEL_PAULA_ADKCON_WORDSYNC) == 0) {
        riegel_destroy(ctx);
        return 1;
    }

    if ((riegel_custom_read16(ctx, RIEGEL_REG_DSKBYTR) & RIEGEL_PAULA_DSKBYTR_DMAON) == 0) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_step(ctx, 1);

    if (riegel_custom_read16(ctx, RIEGEL_REG_DSKDATR) != 0) {
        riegel_destroy(ctx);
        return 1;
    }

    if ((riegel_get_intreq(ctx) & 0x0002u) == 0) {
        riegel_destroy(ctx);
        return 1;
    }

    riegel_destroy(ctx);

    return 0;
}
