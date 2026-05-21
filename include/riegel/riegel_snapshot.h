#ifndef RIEGEL_SNAPSHOT_H
#define RIEGEL_SNAPSHOT_H

#include "riegel_types.h"

typedef struct riegel_snapshot {
    riegel_u64 cycles;
    riegel_u16 intreq;
    riegel_u16 intena;
} riegel_snapshot_t;

bool riegel_save_state(const RiegelContext *ctx, void *buffer, size_t buffer_size);
bool riegel_load_state(RiegelContext *ctx, const void *buffer, size_t buffer_size);

#endif
