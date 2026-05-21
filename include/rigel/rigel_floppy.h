#ifndef RIGEL_FLOPPY_H
#define RIGEL_FLOPPY_H

#include "rigel_types.h"

typedef enum rigel_floppy_drive_id {
    RIGEL_FLOPPY_DRIVE_DF0 = 0,
    RIGEL_FLOPPY_DRIVE_DF1 = 1,
    RIGEL_FLOPPY_DRIVE_DF2 = 2,
    RIGEL_FLOPPY_DRIVE_DF3 = 3
} rigel_floppy_drive_id_t;

typedef struct rigel_floppy_status {
    bool has_media;
    bool motor_on;
    bool ready;
    bool track0;
    bool disk_changed;
    bool write_protected;
    bool dma_active;
    rigel_u8 cylinder;
    rigel_u8 side;
} rigel_floppy_status_t;

rigel_status_t rigel_floppy_insert(
    RigelContext *ctx,
    rigel_floppy_drive_id_t drive,
    const rigel_u8 *data,
    rigel_u32 size
);

void rigel_floppy_eject(RigelContext *ctx, rigel_floppy_drive_id_t drive);
bool rigel_floppy_has_media(const RigelContext *ctx, rigel_floppy_drive_id_t drive);
bool rigel_floppy_get_status(
    const RigelContext *ctx,
    rigel_floppy_drive_id_t drive,
    rigel_floppy_status_t *status
);

#endif
