/**
 * Platform types for the STM32H753 Cortex-M7 target.
 */
#ifndef STM32H7_PLATFORM_TYPES_H_
#define STM32H7_PLATFORM_TYPES_H_

#include <stdint.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

typedef uint32_t PlatformPointerCastType;
#define PRI_PlatformPointerCastType PRIx32

#endif
