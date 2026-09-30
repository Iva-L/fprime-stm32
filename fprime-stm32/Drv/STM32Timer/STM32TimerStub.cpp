// ======================================================================
// \title  STM32TimerStub.cpp
// \author ivanlara
// \brief  HAL boundary stand-in for host-native unit tests. No TIM
//         hardware exists on the build host, so hwReadCounter()/
//         hwSetCompare() operate on a fake, test-injectable counter
//         instead -- no HAL/CMSIS include, no register access. Swapped in
//         for STM32Timer.cpp by Drv/STM32Timer/CMakeLists.txt's
//         register_fprime_ut().
// ======================================================================

#include <fprime-stm32/Drv/STM32Timer/STM32Timer.hpp>

// Observable stub state for unit tests
extern bool Stub_channelArmed = false;      // true once hwArmChannel() has run
extern U32 Stub_lastArmedTarget = 0;        // compare value passed to the most recent
                                            // hwArmChannel()/hwSetCompare() call
extern U32 Stub_hwSetCompareCallCount = 0;  // number of hwSetCompare() reprograms

namespace Stm32 {

void STM32Timer ::hwSelectInstance(TimerInstance instance) {
    this->m_instance = instance;  // no HAL handle to resolve on the host
}

void STM32Timer ::hwArmChannel(U32 target) {
    Stub_channelArmed = true;
    Stub_lastArmedTarget = target;
}

U32 STM32Timer ::hwReadCounter() {
    return this->m_stubCounter;
}

void STM32Timer ::hwSetCompare(U32 target) {
    Stub_lastArmedTarget = target;
    Stub_hwSetCompareCallCount++;
}

}  // namespace Stm32
