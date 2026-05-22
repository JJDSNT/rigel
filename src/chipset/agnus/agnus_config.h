#ifndef RIGEL_AGNUS_CONFIG_H
#define RIGEL_AGNUS_CONFIG_H

/* Video standard — determines frame geometry */
typedef enum agnus_video_std {
    AGNUS_VIDEO_NTSC = 0,
    AGNUS_VIDEO_PAL  = 1,
} agnus_video_std_t;

/* Chip revision — affects available DMA channels and address width */
typedef enum agnus_chip_rev {
    AGNUS_REV_OCS  = 0,  /* 8361/8367 — 512 KB address space */
    AGNUS_REV_ECS  = 1,  /* 8372A     — 1 MB address space   */
    AGNUS_REV_AGA  = 2,  /* 8374 Alice — not yet supported   */
} agnus_chip_rev_t;

enum {
    AGNUS_NTSC_LINE_CLOCKS = 227,
    AGNUS_NTSC_FRAME_LINES = 262,
    AGNUS_PAL_LINE_CLOCKS  = 227,
    AGNUS_PAL_FRAME_LINES  = 312,

    AGNUS_SLOTS_PER_LINE   = 227,
};

#endif
