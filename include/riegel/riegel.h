#ifndef RIEGEL_H
#define RIEGEL_H

#include "riegel_config.h"
#include "riegel_custom.h"
#include "riegel_events.h"
#include "riegel_irq.h"
#include "riegel_mmio.h"
#include "riegel_snapshot.h"
#include "riegel_types.h"

RiegelContext *riegel_create(const riegel_config_t *config);
void riegel_destroy(RiegelContext *ctx);

RiegelChipset *riegel_get_chipset(RiegelContext *ctx);
const RiegelChipset *riegel_get_chipset_const(const RiegelContext *ctx);

void riegel_reset(RiegelContext *ctx);
void riegel_step(RiegelContext *ctx, riegel_u32 cycles);

void riegel_take_snapshot(const RiegelContext *ctx, riegel_snapshot_t *snapshot);

#endif
