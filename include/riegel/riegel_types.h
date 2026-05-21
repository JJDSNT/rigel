#ifndef RIEGEL_TYPES_H
#define RIEGEL_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t riegel_u8;
typedef uint16_t riegel_u16;
typedef uint32_t riegel_u32;
typedef uint64_t riegel_u64;

typedef enum riegel_status {
    RIEGEL_STATUS_OK = 0,
    RIEGEL_STATUS_ERROR = 1
} riegel_status_t;

typedef struct RiegelContext RiegelContext;

#endif
