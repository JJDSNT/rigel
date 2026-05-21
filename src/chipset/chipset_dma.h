#ifndef RIGEL_CHIPSET_DMA_H
#define RIGEL_CHIPSET_DMA_H

#include <stdint.h>
#include <stdbool.h>

typedef struct RigelChipset RigelChipset;

typedef enum rigel_chipset_dma_owner {
    RIGEL_DMA_OWNER_NONE = 0,
    RIGEL_DMA_OWNER_COPPER,
    RIGEL_DMA_OWNER_BLITTER,
    RIGEL_DMA_OWNER_BITPLANE,
    RIGEL_DMA_OWNER_SPRITE,
    RIGEL_DMA_OWNER_AUDIO,
    RIGEL_DMA_OWNER_DISK
} rigel_chipset_dma_owner_t;

typedef struct rigel_chipset_dma_request {
    bool active;
    bool write;

    rigel_chipset_dma_owner_t owner;

    uint32_t address;
    uint16_t data;
} rigel_chipset_dma_request_t;

bool rigel_chipset_dma_poll(
    RigelChipset *chipset,
    rigel_chipset_dma_request_t *out
);

void rigel_chipset_dma_complete(
    RigelChipset *chipset,
    const rigel_chipset_dma_request_t *req,
    uint16_t data
);

#endif
