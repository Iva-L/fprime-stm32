####
# Platform type configuration for the STM32H7 target.
####

register_fprime_config(
    Stm32h7PlatformTypes
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/PlatformTypes.fpp"
    HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/PlatformTypes.h"
    INTERFACE
    BASE_CONFIG
)
