#include "paula/disk.h"

#include <stddef.h>

void disk_reset(disk_state_t *disk)
{
    if (disk == NULL) {
        return;
    }

    disk->inserted = 0;
}
