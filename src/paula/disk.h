#ifndef DISK_H
#define DISK_H

typedef struct disk_state {
    unsigned inserted;
} disk_state_t;

void disk_reset(disk_state_t *disk);

#endif
