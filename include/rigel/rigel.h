#ifndef RIGEL_H
#define RIGEL_H

#include "rigel_audio.h"
#include "rigel_beam.h"
#include "rigel_bus.h"
#include "rigel_cia.h"
#include "rigel_config.h"
#include "rigel_custom.h"
#include "rigel_denise.h"
#include "rigel_events.h"
#include "rigel_floppy.h"
#include "rigel_input.h"
#include "rigel_irq.h"
#include "rigel_keyboard.h"
#include "rigel_mmio.h"
#include "rigel_rtc.h"
#include "rigel_serial.h"
#include "rigel_snapshot.h"
#include "rigel_time.h"
#include "rigel_types.h"

RigelContext *rigel_create(const rigel_config_t *config);
void rigel_destroy(RigelContext *ctx);

/*
 * Wire all chipset-internal connections and apply peripheral config.
 * Called automatically by rigel_create(). Can be called explicitly after
 * modifying the stored config (e.g. changing serial settings at runtime).
 */
void rigel_chipset_wire(RigelContext *ctx);

void rigel_reset(RigelContext *ctx);

void rigel_take_snapshot(const RigelContext *ctx, rigel_snapshot_t *snapshot);

#endif
