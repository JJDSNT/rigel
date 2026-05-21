#ifndef RIGEL_H
#define RIGEL_H

#include "rigel_config.h"
#include "rigel_custom.h"
#include "rigel_events.h"
#include "rigel_irq.h"
#include "rigel_mmio.h"
#include "rigel_snapshot.h"
#include "rigel_types.h"

RigelContext *rigel_create(const rigel_config_t *config);
void rigel_destroy(RigelContext *ctx);

RigelChipset *rigel_get_chipset(RigelContext *ctx);
const RigelChipset *rigel_get_chipset_const(const RigelContext *ctx);

void rigel_reset(RigelContext *ctx);
void rigel_step(RigelContext *ctx, rigel_u32 cycles);

void rigel_take_snapshot(const RigelContext *ctx, rigel_snapshot_t *snapshot);

#endif
