####
# F Prime CMakeLists.txt:
####
if (FPRIME_PLATFORM MATCHES "^stm32" AND NOT STM32_FORCE_STUBBED_DRIVERS)
    set(STM32_I2C_DRIVER_IMPL_SOURCE "${CMAKE_CURRENT_LIST_DIR}/Stm32I2cDriver.cpp")
    set(STM32_I2C_DRIVER_IMPL_DEPENDS FprimeStm32Config FprimeStm32)
else()
    set(STM32_I2C_DRIVER_IMPL_SOURCE "${CMAKE_CURRENT_LIST_DIR}/Stm32I2cDriverStub.cpp")
    set(STM32_I2C_DRIVER_IMPL_DEPENDS FprimeStm32Config)
endif()

register_fprime_module(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/Stm32I2cDriver.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/Stm32I2cDriverCommon.cpp"
    "${STM32_I2C_DRIVER_IMPL_SOURCE}"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/Stm32I2cDriver.hpp"
  DEPENDS
    ${STM32_I2C_DRIVER_IMPL_DEPENDS}
)

### Unit Tests ###
register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/Stm32I2cDriver.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Stm32I2cDriverCommon.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Stm32I2cDriverStub.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/Stm32I2cDriverTestMain.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/Stm32I2cDriverTester.cpp"
    DEPENDS
        STest # For rules-based testing
    UT_AUTO_HELPERS
)