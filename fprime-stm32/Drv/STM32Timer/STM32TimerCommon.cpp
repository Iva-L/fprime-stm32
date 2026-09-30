// ======================================================================
// \title  STM32TimerCommon.cpp
// \author ivanlara
// \brief  Hardware-independent logic for the STM32H7 TIM2 CH2 tick source:
//         shared by the real (stm32h7) and stubbed (host UT) builds.
//         Contains no HAL/CMSIS dependency.
// ======================================================================

#include <fprime-stm32/Drv/STM32Timer/STM32Timer.hpp>

#include <Os/RawTime.hpp>

namespace Stm32 {

STM32Timer ::STM32Timer(const char* const compName)
    : STM32TimerComponentBase(compName),
      m_instance(TimerInstance::Tim2),
      m_periodTicks(0),
      m_nextTarget(0),
      m_tickCount(0),
      m_overrunCount(0),
      m_opened(false),
      m_tickPending(false),
      m_stubCounter(0) {}

STM32Timer ::~STM32Timer() {}

void STM32Timer ::open(TimerInstance instance, U32 periodUs) {
    FW_ASSERT(periodUs > 0);

    this->m_periodTicks = periodUs;  // the timer runs at 1 MHz: 1 tick == 1 us
    this->m_tickPending = false;

    this->hwSelectInstance(instance);
    this->m_nextTarget = this->hwReadCounter() + this->m_periodTicks;

    this->hwArmChannel(this->m_nextTarget);

    this->m_opened = true;
    this->log_ACTIVITY_HI_Configured(periodUs);
}

void STM32Timer ::signalTick() {
    this->m_tickPending = true;
}

void STM32Timer ::poll() {
    if (!this->m_opened || !this->m_tickPending) {
        return;
    }
    this->m_tickPending = false;

    this->m_tickCount++;
    this->tlmWrite_TickCount(this->m_tickCount);

    Os::RawTime timestamp;
    (void)timestamp.now();
    this->CycleOut_out(0, timestamp);

    U32 nextTarget = this->m_nextTarget + this->m_periodTicks;
    const U32 now = this->hwReadCounter();

    if (static_cast<I32>(now - nextTarget) >= 0) {
        const U32 lateUs = now - nextTarget;
        nextTarget = now + this->m_periodTicks;
        this->m_overrunCount++;
        this->tlmWrite_OverrunCount(this->m_overrunCount);
        this->log_WARNING_HI_TickOverrun(lateUs);
    }

    this->m_nextTarget = nextTarget;
    this->hwSetCompare(this->m_nextTarget);
}

}  // namespace Stm32