#include "denise/video.h"

#include <stddef.h>

void video_configure(video_state_t *video, unsigned width, unsigned height)
{
    if (video == NULL) {
        return;
    }

    video->width = width;
    video->height = height;
}
