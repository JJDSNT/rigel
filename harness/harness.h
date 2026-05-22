#ifndef RIGEL_HARNESS_H
#define RIGEL_HARNESS_H

#include <stdbool.h>
#include <stdint.h>

#include "rigel/rigel.h"

/*
 * Harness: Musashi (68000) + Rigel (chipset) wired together for timing tests.
 *
 * Memory map (classic OCS/ECS):
 *   0x000000 - 0x07FFFF  Chip RAM (512KB)
 *   0xBFD000 - 0xBFEFFF  CIA-A/B (not yet wired)
 *   0xC00000 - 0xC7FFFF  Slow RAM (not yet wired)
 *   0xDFF000 - 0xDFF1FF  Custom registers (Rigel)
 *   0xF80000 - 0xFFFFFF  ROM (harness fills with test code)
 */

enum {
    HARNESS_CHIP_RAM_SIZE = 512 * 1024,
    HARNESS_ROM_SIZE      = 512 * 1024,
    HARNESS_ROM_BASE      = 0xF80000
};

typedef struct harness_t harness_t;

harness_t *harness_create(void);
void       harness_destroy(harness_t *h);

/* Load raw 68000 code into ROM at offset from HARNESS_ROM_BASE. */
void harness_load_rom(harness_t *h, const uint8_t *data, uint32_t size);

/* Run until done() returns true or max_cycles elapsed. */
void harness_run(harness_t *h, bool (*done)(harness_t *), uint64_t max_cycles);

/* Accessors for assertions in tests. */
RigelContext *harness_rigel(harness_t *h);
uint64_t      harness_cpu_cycles(const harness_t *h);
uint8_t      *harness_chip_ram(harness_t *h);

#endif
