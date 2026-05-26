#include "paula/disk.h"

#include <stddef.h>

static rigel_u16 disk_read_be16(const rigel_u8 *src)
{
    return (rigel_u16)(((rigel_u16)src[0] << 8) | src[1]);
}

static rigel_u32 disk_track_offset(const FloppyDrive *drive)
{
    rigel_u32 track;

    if (drive == NULL) {
        return 0;
    }

    track = (rigel_u32)((drive->cylinder << 1) | (drive->side ? 1 : 0));
    return track * AMIGA_TRACK_SIZE;
}

static int disk_track_has_sync(const rigel_u8 *src, rigel_u32 len, rigel_u16 sync)
{
    rigel_u32 i;

    if (src == NULL || len < 2) {
        return 0;
    }

    for (i = 0; i + 1 < len; ++i) {
        if (disk_read_be16(src + i) == sync) {
            return 1;
        }
    }

    return 0;
}

static rigel_u32 disk_track_find_sync(const rigel_u8 *src, rigel_u32 len, rigel_u16 sync)
{
    rigel_u32 i;

    if (src == NULL || len < 2) {
        return len;
    }

    for (i = 0; i + 1 < len; ++i) {
        if (disk_read_be16(src + i) == sync) {
            return i;
        }
    }

    return len;
}

static void disk_emit_irq(disk_state_t *disk, rigel_u16 mask)
{
    if (disk == NULL || disk->irq.raise == NULL) {
        return;
    }

    disk->irq.raise(disk->irq.opaque, mask);
}

static void disk_load_dskbytr(disk_state_t *disk, rigel_u16 word)
{
    disk->dskbytr_data = (rigel_u16)(0x8000u | (word & 0x00ffu));
    disk->dskdatr = word;
}

static void disk_maybe_emit_sync(disk_state_t *disk)
{
    if (disk == NULL) {
        return;
    }

    if (!disk->sync_seen || !(disk->adkcon & RIGEL_PAULA_ADKCON_WORDSYNC) || disk->sync_irq_fired) {
        return;
    }

    disk->sync_irq_fired = 1;
    disk_emit_irq(disk, 0x1000u);
}

void disk_reset(disk_state_t *disk)
{
    FloppyDrive *drive;
    rigel_chip_ram_if_t chip_ram;
    paula_disk_irq_sink_t irq;

    if (disk == NULL) {
        return;
    }

    drive = disk->drive;
    chip_ram = disk->chip_ram;
    irq = disk->irq;

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
    disk->dma_src_offset = 0;
    disk->dma_track_len = 0;
    disk->dma_fill_word = 0;
    disk->drive = drive;
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

void disk_set_memory_if(disk_state_t *disk, rigel_chip_ram_if_t chip_ram)
{
    if (disk == NULL) {
        return;
    }

    disk->chip_ram = chip_ram;
}

void disk_set_drive(disk_state_t *disk, FloppyDrive *drive)
{
    if (disk == NULL) {
        return;
    }

    disk->drive = drive;
}

void disk_write_dskpth(disk_state_t *disk, rigel_u16 value)
{
    if (disk == NULL) {
        return;
    }

    disk->dskptr = (disk->dskptr & 0x0000ffffu) | ((rigel_u32)value << 16);
}

void disk_write_dskptl(disk_state_t *disk, rigel_u16 value)
{
    if (disk == NULL) {
        return;
    }

    disk->dskptr = (disk->dskptr & 0xffff0000u) | value;
}

void disk_write_dsksync(disk_state_t *disk, rigel_u16 value)
{
    if (disk == NULL) {
        return;
    }

    disk->dsksync = value;
    disk->sync_irq_fired = 0;
    disk_maybe_emit_sync(disk);
}

void disk_write_adkcon(disk_state_t *disk, rigel_u16 value)
{
    rigel_u16 bits;

    if (disk == NULL) {
        return;
    }

    bits = (rigel_u16)(value & 0x7fffu);

    if ((value & RIGEL_PAULA_ADKCON_SETCLR) != 0) {
        disk->adkcon = (rigel_u16)(disk->adkcon | bits);
    } else {
        disk->adkcon = (rigel_u16)(disk->adkcon & (rigel_u16)(~bits));
    }

    disk_maybe_emit_sync(disk);
}

static void disk_start_dma(disk_state_t *disk, rigel_u16 value)
{
    rigel_u32 len_words;
    rigel_u32 adf_offset;
    rigel_u32 sync_offset;
    rigel_u32 track;

    len_words = (rigel_u32)(value & RIGEL_PAULA_DSKLEN_LEN);

    disk->dsklen = value;
    disk->write_mode = (value & RIGEL_PAULA_DSKLEN_WRITE) != 0;
    disk->dma_active = 1;
    disk->dma_armed = 0;
    disk->sync_seen = 0;
    disk->sync_irq_fired = 0;
    disk->dskbytr_data = 0;
    disk->countdown = 0;
    disk->dskbytr = (rigel_u16)(disk->dskbytr | RIGEL_PAULA_DSKBYTR_DMAON);
    disk->dskbytr = (rigel_u16)(disk->dskbytr & (rigel_u16)(~RIGEL_PAULA_DSKBYTR_WORDSYNC));
    disk->dskdatr = 0;
    disk->dma_ptr_base = disk->dskptr;
    disk->dma_bytes_total = 0;
    disk->dma_bytes_done = 0;
    disk->dma_src_offset = 0;
    disk->dma_track_len = 0;
    disk->dma_fill_word = 0;

    if (disk->write_mode || len_words == 0) {
        return;
    }

    if (disk->drive == NULL || !floppy_has_media(disk->drive)) {
        disk->countdown = RIGEL_PAULA_DISK_FAKE_DMA_CYCLES;
        return;
    }

    adf_offset = disk_track_offset(disk->drive);
    if (adf_offset >= disk->drive->adf_size || AMIGA_TRACK_SIZE > disk->drive->adf_size - adf_offset) {
        disk->countdown = RIGEL_PAULA_DISK_FAKE_DMA_CYCLES;
        return;
    }

    track = adf_offset / AMIGA_TRACK_SIZE;
    if (!floppy_mfm_encode_track(
            disk->dma_track_buf,
            sizeof(disk->dma_track_buf),
            disk->drive->adf + adf_offset,
            track,
            AMIGA_SECTORS_PER_TRACK)) {
        disk->countdown = RIGEL_PAULA_DISK_FAKE_DMA_CYCLES;
        return;
    }

    disk->dma_track_len = AMIGA_MFM_TRACK_SIZE;
    disk->sync_seen = disk_track_has_sync(disk->dma_track_buf, disk->dma_track_len, disk->dsksync);
    sync_offset = disk_track_find_sync(disk->dma_track_buf, disk->dma_track_len, disk->dsksync);
    if (sync_offset + 1 < disk->dma_track_len) {
        disk_load_dskbytr(disk, disk_read_be16(disk->dma_track_buf + sync_offset));
        disk->dma_src_offset = sync_offset;
    }

    disk_maybe_emit_sync(disk);
    disk->dma_bytes_total = len_words << 1;
}

void disk_write_dsklen(disk_state_t *disk, rigel_u16 value)
{
    if (disk == NULL) {
        return;
    }

    if ((value & RIGEL_PAULA_DSKLEN_DMAEN) == 0) {
        disk->dsklen = value;
        disk->dma_active = 0;
        disk->dma_armed = 0;
        disk->armed_dsklen = 0;
        disk->countdown = 0;
        disk->sync_seen = 0;
        disk->sync_irq_fired = 0;
        disk->dskbytr_data = 0;
        disk->dskbytr = (rigel_u16)(disk->dskbytr & (rigel_u16)(~RIGEL_PAULA_DSKBYTR_DMAON));
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

rigel_u16 disk_read_dskbytr(disk_state_t *disk)
{
    rigel_u16 value;

    if (disk == NULL) {
        return 0;
    }

    value = (rigel_u16)(disk->dskbytr_data & 0x80ffu);

    if (disk->sync_seen) {
        value = (rigel_u16)(value | RIGEL_PAULA_DSKBYTR_WORDSYNC);
    }

    if (disk->dma_active) {
        value = (rigel_u16)(value | RIGEL_PAULA_DSKBYTR_DMAON);
    }

    disk->dskbytr_data = (rigel_u16)(disk->dskbytr_data & (rigel_u16)(~0x8000u));
    return value;
}

rigel_u16 disk_read_dskdatr(const disk_state_t *disk)
{
    if (disk == NULL) {
        return 0;
    }

    return disk->dskdatr;
}

void disk_step(disk_state_t *disk, rigel_u32 cycles)
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
    disk->dskbytr = (rigel_u16)(disk->dskbytr & (rigel_u16)(~RIGEL_PAULA_DSKBYTR_DMAON));
    disk->dsklen = (rigel_u16)(disk->dsklen & (rigel_u16)(~RIGEL_PAULA_DSKLEN_DMAEN));
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
    rigel_u32 dst;
    rigel_u16 word;

    if (!disk_dma_wants_service(disk)) {
        return;
    }

    if (disk->chip_ram.write16 == NULL || disk->dma_track_len < 2) {
        return;
    }

    dst = disk->dma_ptr_base + disk->dma_bytes_done;
    if (disk->dma_src_offset + 1 >= disk->dma_track_len) {
        disk->dma_src_offset = 0;
    }

    word = disk_read_be16(disk->dma_track_buf + disk->dma_src_offset);

    disk->chip_ram.write16(disk->chip_ram.opaque, dst, word);
    disk_load_dskbytr(disk, word);
    disk->dma_bytes_done += 2;
    disk->dma_src_offset = (disk->dma_src_offset + 2) % disk->dma_track_len;

    if (disk->dma_bytes_done < disk->dma_bytes_total) {
        return;
    }

    disk->dma_active = 0;
    disk->dma_armed = 0;
    disk->armed_dsklen = 0;
    disk->sync_seen = 0;
    disk->sync_irq_fired = 0;
    disk->dskbytr = (rigel_u16)(disk->dskbytr & (rigel_u16)(~RIGEL_PAULA_DSKBYTR_DMAON));
    disk->dsklen = (rigel_u16)(disk->dsklen & (rigel_u16)(~RIGEL_PAULA_DSKLEN_DMAEN));
    disk->dskptr = disk->dma_ptr_base + disk->dma_bytes_total;
    disk_emit_irq(disk, 0x0002u);
}

rigel_u32 disk_cycles_to_next_event(const disk_state_t *disk)
{
    if (disk == NULL || !disk->dma_active) {
        return 0xFFFFFFFFu;
    }

    /* Countdown phase: fake-DMA delay until completion IRQ. */
    if (disk->countdown > 0u && disk->dma_bytes_total == 0u) {
        return disk->countdown;
    }

    return 0xFFFFFFFFu;
}
