####
# GNU Arm Embedded toolchain for the STM32H753XI-EVAL2.
####

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(FPRIME_PLATFORM stm32h7 CACHE INTERNAL "F Prime platform" FORCE)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(STM32H7_CPU_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")

set(CMAKE_C_FLAGS_INIT
    "${STM32H7_CPU_FLAGS} -ffunction-sections -fdata-sections -g3 -gdwarf-4"
)
set(CMAKE_CXX_FLAGS_INIT
    "${STM32H7_CPU_FLAGS} -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -g3 -gdwarf-4 -include${CMAKE_CURRENT_LIST_DIR}/stm32h7-format-macros.h"
)
set(CMAKE_ASM_FLAGS_INIT "${STM32H7_CPU_FLAGS} -g3 -gdwarf-4")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
