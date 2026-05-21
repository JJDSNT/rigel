#ifndef DISK_H
#define DISK_H

#include "riegel/riegel_config.h"
#include "riegel/riegel_types.h"

enum {
    RIEGEL_PAULA_DSKLEN_DMAEN = 0x8000u,
    RIEGEL_PAULA_DSKLEN_WRITE = 0x4000u,
    RIEGEL_PAULA_DSKLEN_LEN = 0x3fffu,
    RIEGEL_PAULA_DSKBYTR_DMAON = 0x2000u,
    RIEGEL_PAULA_DSKBYTR_WORDSYNC = 0x0800u,
    RIEGEL_PAULA_ADKCON_SETCLR = 0x8000u,
    RIEGEL_PAULA_ADKCON_WORDSYNC = 0x0400u,
    RIEGEL_PAULA_DISK_FAKE_DMA_CYCLES = 46000u
};

typedef void (*paula_disk_irq_raise_fn)(void *opaque, riegel_u16 mask);

typedef struct paula_disk_irq_sink {
    void *opaque;
    paula_disk_irq_raise_fn raise;
} paula_disk_irq_sink_t;

typedef struct disk_state {
    unsigned inserted;
    riegel_u32 dskptr;
    riegel_u16 dsklen;
    riegel_u16 dskbytr;
    riegel_u16 dskdatr;
    riegel_u16 dsksync;
    riegel_u16 adkcon;
    int dma_active;
    int dma_armed;
    int write_mode;
    int sync_seen;
    int sync_irq_fired;
    riegel_u16 dskbytr_data;
    riegel_u16 armed_dsklen;
    riegel_u32 countdown;
    riegel_u32 dma_ptr_base;
    riegel_u32 dma_bytes_total;
    riegel_u32 dma_bytes_done;
    riegel_u16 dma_fill_word;
    riegel_chip_ram_if_t chip_ram;
    paula_disk_irq_sink_t irq;
} disk_state_t;

void disk_reset(disk_state_t *disk);
void disk_set_irq_sink(disk_state_t *disk, paula_disk_irq_sink_t sink);
void disk_set_memory_if(disk_state_t *disk, riegel_chip_ram_if_t chip_ram);
void disk_set_inserted(disk_state_t *disk, int inserted);
void disk_write_dskpth(disk_state_t *disk, riegel_u16 value);
void disk_write_dskptl(disk_state_t *disk, riegel_u16 value);
void disk_write_dsklen(disk_state_t *disk, riegel_u16 value);
void disk_write_dsksync(disk_state_t *disk, riegel_u16 value);
void disk_write_adkcon(disk_state_t *disk, riegel_u16 value);
riegel_u16 disk_read_dskbytr(disk_state_t *disk);
riegel_u16 disk_read_dskdatr(const disk_state_t *disk);
void disk_step(disk_state_t *disk, riegel_u32 cycles);
int disk_dma_wants_service(const disk_state_t *disk);
void disk_dma_service_grant(disk_state_t *disk);

#endif
