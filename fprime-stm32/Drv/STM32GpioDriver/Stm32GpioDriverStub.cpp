// ======================================================================
// \title  Stm32GpioDriverStub.cpp
// \author ivanlara
// \brief  HAL boundary stand-in for host-native unit tests. No STM32
//         hardware exists on the build host, so every call here is a
//         fixed-behavior stub -- no HAL/CMSIS include, no register access.
//         Swapped in for Stm32GpioDriver.cpp by
//         Drv/STM32GpioDriver/CMakeLists.txt's register_fprime_ut().
// ======================================================================

#include <fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriver.hpp>

// Injectable stub state for unit tests
extern bool Stub_hwConfigurePin = true;    // HAL_GPIO_Init()/mode-verify success or failure
extern bool Stub_hwReadPin = false;        // simulated input pin level

// Observable stub state for unit tests
extern bool Stub_lastWrittenPinHigh = false;  // level most recently passed to hwWritePin()
extern U32 Stub_hwWriteCallCount = 0;         // number of hwWritePin() calls

namespace Stm32 {

bool Stm32GpioDriver ::hwConfigurePin(GpioPort port, U16 pin, Fw::Direction mode, Fw::Logic defaultState) {
    return Stub_hwConfigurePin;
}

bool Stm32GpioDriver ::hwReadPin(GpioPort port, U16 pin) {
    return Stub_hwReadPin;
}

void Stm32GpioDriver ::hwWritePin(GpioPort port, U16 pin, bool high) {
    Stub_lastWrittenPinHigh = high;
    Stub_hwWriteCallCount++;
}

}  // namespace Stm32