#include "paula/disk.h"

#include <stddef.h>

static void disk_emit_irq(disk_state_t *disk, riegel_u16 mask)
{
    if (disk == NULL || disk->irq.raise == NULL) {
        return;
    }

    disk->irq.raise(disk->irq.opaque, mask);
}

static void disk_load_dskbytr(disk_state_t *disk, riegel_u16 word)
{
    disk->dskbytr_data = (riegel_u16)(0x8000u | (word & 0x00ffu));
    disk->dskdatr = word;
}

static void disk_maybe_emit_sync(disk_state_t *disk)
{
    if (disk == NULL) {
        return;
    }

    if (!disk->sync_seen || !(disk->adkcon & RIEGEL_PAULA_ADKCON_WORDSYNC) || disk->sync_irq_fired) {
        return;
    }

    disk->sync_irq_fired = 1;
    disk_emit_irq(disk, 0x1000u);
}

void disk_reset(disk_state_t *disk)
{
    unsigned inserted;
    riegel_chip_ram_if_t chip_ram;
    paula_disk_irq_sink_t irq;

    if (disk == NULL) {
        return;
    }

    inserted = disk->inserted;
    chip_ram = disk->chip_ram;
    irq = disk->irq;

    disk->inserted = inserted;
    disk->dskptr = 0;
    disk->dsklen = 0;
    disk->dskbytr = 0;
    disk->dskdatr = 0;
    disk->dsksync = 0x4489u;
    disk->adkcon = 0;
    disk->dma_active = 0;
    disk->dma_armed = 0;
    disk->write_mode = 0;
    disk->sync_seen = 0;
    disk->sync_irq_fired = 0;
    disk->dskbytr_data = 0;
    disk->armed_dsklen = 0;
    disk->countdown = 0;
    disk->dma_ptr_base = 0;
    disk->dma_bytes_total = 0;
    disk->dma_bytes_done = 0;
    disk->dma_fill_word = 0;
    disk->chip_ram = chip_ram;
    disk->irq = irq;
}

void disk_set_irq_sink(disk_state_t *disk, paula_disk_irq_sink_t sink)
{
    if (disk == NULL) {
        return;
    }

    disk->irq = sink;
}

void disk_set_memory_if(disk_state_t *disk, riegel_chip_ram_if_t chip_ram)
{
    if (disk == NULL) {
        return;
    }

    disk->chip_ram = chip_ram;
}

void disk_set_inserted(disk_state_t *disk, int inserted)
{
    if (disk == NULL) {
        return;
    }

    disk->inserted = inserted != 0;
}

void disk_write_dskpth(disk_state_t *disk, riegel_u16 value)
{
    if (disk == NULL) {
        return;
    }

    disk->dskptr = (disk->dskptr & 0x0000ffffu) | ((riegel_u32)value << 16);
}

void disk_write_dskptl(disk_state_t *disk, riegel_u16 value)
{
    if (disk == NULL) {
        return;
    }

    disk->dskptr = (disk->dskptr & 0xffff0000u) | value;
}

void disk_write_dsksync(disk_state_t *disk, riegel_u16 value)
{
    if (disk == NULL) {
        return;
    }

    disk->dsksync = value;
    disk->sync_irq_fired = 0;
    disk_maybe_emit_sync(disk);
}

void disk_write_adkcon(disk_state_t *disk, riegel_u16 value)
{
    riegel_u16 bits;

    if (disk == NULL) {
        return;
    }

    bits = (riegel_u16)(value & 0x7fffu);

    if ((value & RIEGEL_PAULA_ADKCON_SETCLR) != 0) {
        disk->adkcon = (riegel_u16)(disk->adkcon | bits);
    } else {
        disk->adkcon = (riegel_u16)(disk->adkcon & (riegel_u16)(~bits));
    }

    disk_maybe_emit_sync(disk);
}

static void disk_start_dma(disk_state_t *disk, riegel_u16 value)
{
    riegel_u32 len_words;

    len_words = (riegel_u32)(value & RIEGEL_PAULA_DSKLEN_LEN);

    disk->dsklen = value;
    disk->write_mode = (value & RIEGEL_PAULA_DSKLEN_WRITE) != 0;
    disk->dma_active = 1;
    disk->dma_armed = 0;
    disk->sync_seen = 0;
    disk->sync_irq_fired = 0;
    disk->dskbytr_data = 0;
    disk->countdown = 0;
    disk->dskbytr = (riegel_u16)(disk->dskbytr | RIEGEL_PAULA_DSKBYTR_DMAON);
    disk->dskbytr = (riegel_u16)(disk->dskbytr & (riegel_u16)(~RIEGEL_PAULA_DSKBYTR_WORDSYNC));
    disk->dskdatr = 0;
    disk->dma_ptr_base = disk->dskptr;
    disk->dma_bytes_total = 0;
    disk->dma_bytes_done = 0;
    disk->dma_fill_word = 0;

    if (disk->write_mode || len_words == 0) {
        return;
    }

    if (!disk->inserted) {
        disk->countdown = RIEGEL_PAULA_DISK_FAKE_DMA_CYCLES;
        return;
    }

    disk->dma_bytes_total = len_words << 1;
}

void disk_write_dsklen(disk_state_t *disk, riegel_u16 value)
{
    if (disk == NULL) {
        return;
    }

    if ((value & RIEGEL_PAULA_DSKLEN_DMAEN) == 0) {
        disk->dsklen = value;
        disk->dma_active = 0;
        disk->dma_armed = 0;
        disk->armed_dsklen = 0;
        disk->countdown = 0;
        disk->sync_seen = 0;
        disk->sync_irq_fired = 0;
        disk->dskbytr_data = 0;
        disk->dskbytr = (riegel_u16)(disk->dskbytr & (riegel_u16)(~RIEGEL_PAULA_DSKBYTR_DMAON));
        return;
    }

    if (!disk->dma_armed) {
        disk->dsklen = value;
        disk->armed_dsklen = value;
        disk->dma_armed = 1;
        return;
    }

    disk_start_dma(disk, value);
}

riegel_u16 disk_read_dskbytr(disk_state_t *disk)
{
    riegel_u16 value;

    if (disk == NULL) {
        return 0;
    }

    value = (riegel_u16)(disk->dskbytr_data & 0x80ffu);

    if (disk->sync_seen) {
        value = (riegel_u16)(value | RIEGEL_PAULA_DSKBYTR_WORDSYNC);
    }

    if (disk->dma_active) {
        value = (riegel_u16)(value | RIEGEL_PAULA_DSKBYTR_DMAON);
    }

    disk->dskbytr_data = (riegel_u16)(disk->dskbytr_data & (riegel_u16)(~0x8000u));
    return value;
}

riegel_u16 disk_read_dskdatr(const disk_state_t *disk)
{
    if (disk == NULL) {
        return 0;
    }

    return disk->dskdatr;
}

void disk_step(disk_state_t *disk, riegel_u32 cycles)
{
    if (disk == NULL) {
        return;
    }

    if (!disk->dma_active || disk->dma_bytes_total > 0 || disk->countdown == 0) {
        return;
    }

    if (disk->countdown > cycles) {
        disk->countdown -= cycles;
        return;
    }

    disk->countdown = 0;
    disk->dma_active = 0;
    disk->dma_armed = 0;
    disk->armed_dsklen = 0;
    disk->sync_seen = 0;
    disk->sync_irq_fired = 0;
    disk->dskbytr = (riegel_u16)(disk->dskbytr & (riegel_u16)(~RIEGEL_PAULA_DSKBYTR_DMAON));
    disk->dsklen = (riegel_u16)(disk->dsklen & (riegel_u16)(~RIEGEL_PAULA_DSKLEN_DMAEN));
    disk_emit_irq(disk, 0x0002u);
}

int disk_dma_wants_service(const disk_state_t *disk)
{
    if (disk == NULL) {
        return 0;
    }

    if (!disk->dma_active || disk->write_mode) {
        return 0;
    }

    if (disk->dma_bytes_done >= disk->dma_bytes_total) {
        return 0;
    }

    return 1;
}

void disk_dma_service_grant(disk_state_t *disk)
{
    riegel_u32 dst;
    riegel_u16 word;

    if (!disk_dma_wants_service(disk)) {
        return;
    }

    if (disk->chip_ram.write16 == NULL) {
        return;
    }

    dst = disk->dma_ptr_base + disk->dma_bytes_done;
    word = disk->dma_fill_word;

    disk->chip_ram.write16(disk->chip_ram.opaque, dst, word);
    disk_load_dskbytr(disk, word);
    disk->dma_bytes_done += 2;

    if (disk->dma_bytes_done < disk->dma_bytes_total) {
        return;
    }

    disk->dma_active = 0;
    disk->dma_armed = 0;
    disk->armed_dsklen = 0;
    disk->sync_seen = 0;
    disk->sync_irq_fired = 0;
    disk->dskbytr = (riegel_u16)(disk->dskbytr & (riegel_u16)(~RIEGEL_PAULA_DSKBYTR_DMAON));
    disk->dsklen = (riegel_u16)(disk->dsklen & (riegel_u16)(~RIEGEL_PAULA_DSKLEN_DMAEN));
    disk->dskptr = disk->dma_ptr_base + disk->dma_bytes_total;
    disk_emit_irq(disk, 0x0002u);
}
