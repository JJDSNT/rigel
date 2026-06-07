#include "bitplane_fetch.h"
#include "bitplane_pointers.h"

#if RIGEL_ENABLE_STDLIB_ENV
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#endif

void bitplane_fetch_reset(bitplane_fetch_state_t *f)
{
    for (unsigned i = 0; i < 6; i++)
        f->data[i] = 0;
}

void bitplane_fetch_step(bitplane_fetch_state_t *f,
                         bitplane_pointers_t *ptrs,
                         unsigned plane,
                         rigel_chip_ram_if_t mem)
{
    if (plane >= 6 || mem.read16 == NULL) return;
#if RIGEL_ENABLE_STDLIB_ENV
    {
        static int probe_enabled = -1;
        static unsigned probe_count = 0u;
        if (probe_enabled < 0) {
            const char *e = getenv("RIGEL_BPL_FETCH_PROBE");
            probe_enabled = (e != NULL && e[0] != '\0' && e[0] != '0') ? 1 : 0;
        }
        if (probe_enabled && probe_count < 32u) {
            rigel_u32 addr_before = ptrs->bplpt[plane];
            rigel_u16 result = mem.read16(mem.opaque, addr_before);
            f->data[plane] = result;
            bplpt_advance(ptrs, plane);
            printf("[BPL-FETCH-PROBE] plane=%u addr=%06x result=%04x opaque=%p read16=%p\n",
                   plane, (unsigned)addr_before, (unsigned)result,
                   mem.opaque, (void *)(uintptr_t)(mem.read16));
            probe_count++;
            return;
        }
    }
#endif
    f->data[plane] = mem.read16(mem.opaque, ptrs->bplpt[plane]);
    bplpt_advance(ptrs, plane);
}

rigel_u16 bitplane_fetch_data(const bitplane_fetch_state_t *f, unsigned plane)
{
    if (plane >= 6) return 0;
    return f->data[plane];
}
