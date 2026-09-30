####
# F prime CMakeLists.txt:
#
# register_fprime_module() is always registered (needed so the module's
# .fpp is discoverable during dependency-info generation on every
# platform, including a host-native `fprime-util check` run) -- only the
# SOURCES/DEPENDS choice below is platform-conditional. On any stm32*
# target it links the real, HAL-dependent Stm32UartDriver.cpp; everywhere
# else (including host UT) it links the HAL-free Stm32UartDriverStub.cpp.
# See lib/fprime-stm32/README.md for the Common/Real/Stub convention.
####
if (FPRIME_PLATFORM MATCHES "^stm32" AND NOT STM32_FORCE_STUBBED_DRIVERS)
    set(STM32_UART_DRIVER_IMPL_SOURCE "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriver.cpp")
    set(STM32_UART_DRIVER_IMPL_DEPENDS Os FprimeStm32Config FprimeStm32 FprimeStm32CoreCortexM7)
else()
    set(STM32_UART_DRIVER_IMPL_SOURCE "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriverStub.cpp")
    set(STM32_UART_DRIVER_IMPL_DEPENDS Os FprimeStm32Config)
endif()

register_fprime_module(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriver.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriverCommon.cpp"
    "${STM32_UART_DRIVER_IMPL_SOURCE}"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriver.hpp"
  DEPENDS
    ${STM32_UART_DRIVER_IMPL_DEPENDS}
)

register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriver.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriverCommon.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Stm32UartDriverStub.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/Stm32UartDriverTestMain.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/Stm32UartDriverTester.cpp"
    DEPENDS
        Os
        FprimeStm32Config
        STest # For rules-based testing
    UT_AUTO_HELPERS
)
