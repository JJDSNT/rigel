#ifndef RIGEL_MMIO_H
#define RIGEL_MMIO_H

#include "rigel_types.h"

typedef enum rigel_mmio_result {
    RIGEL_MMIO_UNMAPPED = 0,
    RIGEL_MMIO_HANDLED = 1,
    RIGEL_MMIO_UNSUPPORTED = 2
} rigel_mmio_result_t;

/*
 * Decode a CPU-visible classic Amiga address inside Rigel.
 *
 * addr is the M68K-visible physical address, size is in bytes, and values are
 * logical big-endian M68K values.  UNMAPPED means that the host remains
 * responsible for the address; UNSUPPORTED means that Rigel owns the address
 * but the requested access width/alignment has no implemented semantics.
 */
rigel_mmio_result_t rigel_mmio_read(RigelContext *ctx, rigel_u32 addr,
                                    rigel_u8 size, rigel_u32 *value);
rigel_mmio_result_t rigel_mmio_write(RigelContext *ctx, rigel_u32 addr,
                                     rigel_u8 size, rigel_u32 value);

rigel_u16 rigel_custom_read16(RigelContext *ctx, rigel_u32 addr);
void rigel_custom_write16(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

#endif
