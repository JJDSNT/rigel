#ifndef RIGEL_SNAPSHOT_H
#define RIGEL_SNAPSHOT_H

#include "rigel_types.h"

/*
 * INCOMPLETE — captures only a subset of chipset state. Not suitable for full
 * save/restore. Use rigel_take_snapshot for lightweight inspection only.
 * A complete state serialisation API is planned once internal state stabilises.
 */
typedef struct rigel_snapshot {
    rigel_u64 cycles;
    rigel_u16 intreq;
    rigel_u16 intena;
} rigel_snapshot_t;

bool rigel_save_state(const RigelContext *ctx, void *buffer, size_t buffer_size);
bool rigel_load_state(RigelContext *ctx, const void *buffer, size_t buffer_size);

#endif
