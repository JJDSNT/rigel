#include "pixel_pipeline.h"
#include "priority.h"
#include "ham.h"
#include "ehb.h"
#include "dualpf.h"

/* TODO(render): wire up HAM/EHB/dualpf paths and sprite composite */

rigel_u32 denise_pixel_pipeline_run(const denise_pixel_inputs_t *in,
                                    const rigel_u32 palette[32])
{
    uint8_t color_index;

    /* TODO(render): implement full pipeline */
    color_index = in->plane_bits & 0x1Fu;
    return palette[color_index] | 0xFF000000u;
}
