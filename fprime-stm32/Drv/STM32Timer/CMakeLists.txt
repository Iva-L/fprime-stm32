####
# F prime CMakeLists.txt:
#
# register_fprime_library() is always registered (needed so the module's
# .fpp is discoverable during dependency-info generation on every
# platform, including a host-native `fprime-util check` run) -- only the
# SOURCES/DEPENDS choice below is platform-conditional. On the stm32h7
# target it links the real, HAL-dependent STM32Timer.cpp; everywhere else
# (including host UT) it links the HAL-free STM32TimerStub.cpp. See
# lib/fprime-stm32/README.md for the Common/Real/Stub convention.
####
if (FPRIME_PLATFORM MATCHES "^stm32" AND NOT STM32_FORCE_STUBBED_DRIVERS)
    set(STM32_TIMER_IMPL_SOURCE "${CMAKE_CURRENT_LIST_DIR}/STM32Timer.cpp")
    set(STM32_TIMER_IMPL_DEPENDS FprimeStm32)
else()
    set(STM32_TIMER_IMPL_SOURCE "${CMAKE_CURRENT_LIST_DIR}/STM32TimerStub.cpp")
    set(STM32_TIMER_IMPL_DEPENDS)
endif()

register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/STM32Timer.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/STM32TimerCommon.cpp"
        "${STM32_TIMER_IMPL_SOURCE}"
    DEPENDS
        FprimeStm32Config
        ${STM32_TIMER_IMPL_DEPENDS}
)

register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/STM32Timer.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/STM32TimerCommon.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/STM32TimerStub.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/STM32TimerTestMain.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/STM32TimerTester.cpp"
    DEPENDS
    FprimeStm32Config
    STest # For rules-based testing
    UT_AUTO_HELPERS
)
