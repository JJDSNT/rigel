#ifndef VIDEO_H
#define VIDEO_H

typedef struct video_state {
    unsigned width;
    unsigned height;
} video_state_t;

void video_configure(video_state_t *video, unsigned width, unsigned height);

#endif
