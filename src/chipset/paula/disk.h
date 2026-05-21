#ifndef DISK_H
#define DISK_H

#include "floppy/floppy_drive.h"
#include "floppy/floppy_mfm.h"
#include "rigel/rigel_config.h"
#include "rigel/rigel_types.h"

enum {
    RIGEL_PAULA_DSKLEN_DMAEN = 0x8000u,
    RIGEL_PAULA_DSKLEN_WRITE = 0x4000u,
    RIGEL_PAULA_DSKLEN_LEN = 0x3fffu,
    RIGEL_PAULA_DSKBYTR_DMAON = 0x2000u,
    RIGEL_PAULA_DSKBYTR_WORDSYNC = 0x0800u,
    RIGEL_PAULA_ADKCON_SETCLR = 0x8000u,
    RIGEL_PAULA_ADKCON_WORDSYNC = 0x0400u,
    RIGEL_PAULA_DISK_FAKE_DMA_CYCLES = 46000u
};

typedef void (*paula_disk_irq_raise_fn)(void *opaque, rigel_u16 mask);

typedef struct paula_disk_irq_sink {
    void *opaque;
    paula_disk_irq_raise_fn raise;
} paula_disk_irq_sink_t;

typedef struct disk_state {
    rigel_u32 dskptr;
    rigel_u16 dsklen;
    rigel_u16 dskbytr;
    rigel_u16 dskdatr;
    rigel_u16 dsksync;
    rigel_u16 adkcon;
    int dma_active;
    int dma_armed;
    int write_mode;
    int sync_seen;
    int sync_irq_fired;
    rigel_u16 dskbytr_data;
    rigel_u16 armed_dsklen;
    rigel_u32 countdown;
    rigel_u32 dma_ptr_base;
    rigel_u32 dma_bytes_total;
    rigel_u32 dma_bytes_done;
    rigel_u32 dma_src_offset;
    rigel_u32 dma_track_len;
    rigel_u16 dma_fill_word;
    rigel_u8 dma_track_buf[RIGEL_FLOPPY_MFM_TRACK_BYTES_PAL];
    FloppyDrive *drive;
    rigel_chip_ram_if_t chip_ram;
    paula_disk_irq_sink_t irq;
} disk_state_t;

void disk_reset(disk_state_t *disk);
void disk_set_irq_sink(disk_state_t *disk, paula_disk_irq_sink_t sink);
void disk_set_memory_if(disk_state_t *disk, rigel_chip_ram_if_t chip_ram);
void disk_set_drive(disk_state_t *disk, FloppyDrive *drive);
void disk_write_dskpth(disk_state_t *disk, rigel_u16 value);
void disk_write_dskptl(disk_state_t *disk, rigel_u16 value);
void disk_write_dsklen(disk_state_t *disk, rigel_u16 value);
void disk_write_dsksync(disk_state_t *disk, rigel_u16 value);
void disk_write_adkcon(disk_state_t *disk, rigel_u16 value);
rigel_u16 disk_read_dskbytr(disk_state_t *disk);
rigel_u16 disk_read_dskdatr(const disk_state_t *disk);
void disk_step(disk_state_t *disk, rigel_u32 cycles);
int disk_dma_wants_service(const disk_state_t *disk);
void disk_dma_service_grant(disk_state_t *disk);

#endif
