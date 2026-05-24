#ifndef RIGEL_TYPES_H
#define RIGEL_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int16_t  rigel_i16;
typedef uint8_t  rigel_u8;
typedef uint16_t rigel_u16;
typedef uint32_t rigel_u32;
typedef uint64_t rigel_u64;

typedef uint64_t rigel_cycle_t;

typedef enum rigel_status {
    RIGEL_STATUS_OK = 0,
    RIGEL_STATUS_ERROR = 1
} rigel_status_t;

typedef struct RigelContext RigelContext;
typedef struct RigelChipset RigelChipset;

#endif
