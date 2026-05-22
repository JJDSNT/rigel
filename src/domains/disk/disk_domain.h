#ifndef RIGEL_DISK_DOMAIN_H
#define RIGEL_DISK_DOMAIN_H

#include "paula/disk.h"
#include "rigel/rigel_custom.h"

typedef struct RigelContext RigelContext;

bool rigel_disk_domain_owns_reg(rigel_u32 addr);
void rigel_disk_domain_reset(disk_state_t *disk);
void rigel_disk_domain_step(disk_state_t *disk, rigel_u32 cycles);
void rigel_disk_domain_step_slot(RigelContext *ctx);
int rigel_disk_domain_dma_wants_service(const disk_state_t *disk);
void rigel_disk_domain_dma_service_grant(disk_state_t *disk);
rigel_u16 rigel_disk_domain_read_reg(const disk_state_t *disk, rigel_u32 addr);
void rigel_disk_domain_write_reg(disk_state_t *disk, rigel_u32 addr, rigel_u16 value);

#endif
