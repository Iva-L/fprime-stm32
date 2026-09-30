// ======================================================================
// \title  STM32TimerTester.cpp
// \author ivanlara
// \brief  cpp file for STM32Timer component test harness implementation class
// ======================================================================

#include "STM32TimerTester.hpp"

// Stub_* globals defined in STM32TimerStub.cpp -- declared here so this
// Tester can verify STM32TimerCommon.cpp's open()/poll() arithmetic actually
// reached the HAL boundary with the right values, not just that some HAL
// call happened.
extern bool Stub_channelArmed;
extern U32 Stub_lastArmedTarget;
extern U32 Stub_hwSetCompareCallCount;

namespace Stm32 {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

STM32TimerTester ::STM32TimerTester()
    : STM32TimerGTestBase("STM32TimerTester", STM32TimerTester::MAX_HISTORY_SIZE), component("STM32Timer") {
    this->resetStubState();
    this->initComponents();
    this->connectPorts();
}

STM32TimerTester ::~STM32TimerTester() {
    this->component.deinit();
}

void STM32TimerTester ::resetStubState() {
    Stub_channelArmed = false;
    Stub_lastArmedTarget = 0;
    Stub_hwSetCompareCallCount = 0;
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void STM32TimerTester ::testOpenArms() {
    this->component.open(Stm32::TimerInstance::Tim2, 1000);
    ASSERT_EVENTS_Configured_SIZE(1);
    ASSERT_EVENTS_Configured(0, 1000);
    ASSERT_EQ(this->component.m_nextTarget, 1000u);

    // hwArmChannel() must have been called exactly once, with the same
    // initial compare target Common.cpp computed.
    ASSERT_EQ(Stub_channelArmed, true);
    ASSERT_EQ(Stub_lastArmedTarget, 1000u);
    ASSERT_EQ(Stub_hwSetCompareCallCount, 0u);  // only poll() reprograms the compare
}

void STM32TimerTester ::testPollWithoutTick() {
    this->component.open(Stm32::TimerInstance::Tim2, 1000);
    this->component.poll();
    ASSERT_from_CycleOut_SIZE(0);
    ASSERT_TLM_TickCount_SIZE(0);

    // No tick pending: poll() must return before touching the HAL boundary
    // again (hwSetCompare() is only reached past the tick-pending check).
    ASSERT_EQ(Stub_hwSetCompareCallCount, 0u);
}

void STM32TimerTester ::testPollWithTick() {
    this->component.open(Stm32::TimerInstance::Tim2, 1000);
    this->component.signalTick();
    this->component.poll();
    ASSERT_from_CycleOut_SIZE(1);
    ASSERT_TLM_TickCount_SIZE(1);
    ASSERT_TLM_TickCount(0, 1);
    ASSERT_EVENTS_TickOverrun_SIZE(0);

    // poll() reprograms the compare once, relative to the target that just
    // fired (1000 + periodTicks(1000) = 2000), not relative to "now".
    ASSERT_EQ(Stub_hwSetCompareCallCount, 1u);
    ASSERT_EQ(Stub_lastArmedTarget, 2000u);
    ASSERT_EQ(this->component.m_nextTarget, 2000u);
}

void STM32TimerTester ::testOverrunDetection() {
    this->component.open(Stm32::TimerInstance::Tim2, 100);
    // Advance the fake counter well past the armed compare target so poll()
    // detects the target already elapsed.
    this->component.m_stubCounter = 500;
    this->component.signalTick();
    this->component.poll();
    ASSERT_EVENTS_TickOverrun_SIZE(1);
    ASSERT_TLM_OverrunCount_SIZE(1);
    ASSERT_TLM_OverrunCount(0, 1);

    // Resynced to now + periodTicks (500 + 100 = 600), reprogrammed exactly
    // once even on the overrun path.
    ASSERT_EQ(Stub_hwSetCompareCallCount, 1u);
    ASSERT_EQ(Stub_lastArmedTarget, 600u);
    ASSERT_EQ(this->component.m_nextTarget, 600u);
}

void STM32TimerTester ::testPollWithTickNoOverrunAtExactBoundary() {
    // now == nextTarget (elapsed difference of exactly 0) must NOT count as
    // an overrun: the real driver's signed-difference check is `>= 0`, so
    // this boundary is the one case most likely to regress if that
    // comparison is ever "fixed" to `> 0` by mistake.
    this->component.open(Stm32::TimerInstance::Tim2, 1000);
    this->component.signalTick();
    this->component.m_stubCounter = 2000;  // exactly the next target poll() will compute
    this->component.poll();

    ASSERT_EVENTS_TickOverrun_SIZE(1);
    ASSERT_TLM_OverrunCount(0, 1);
}

void STM32TimerTester ::testTwoInstancesDoNotInterfere() {
    this->component.open(Stm32::TimerInstance::Tim2, 1000);

    // A second, freestanding instance on a different physical timer.
    STM32Timer secondComponent("STM32TimerSecond");
    secondComponent.open(Stm32::TimerInstance::Tim3, 500);

    // The first instance must still be fully functional, unaffected by the
    // second instance's later open().
    this->component.signalTick();
    this->component.poll();
    ASSERT_from_CycleOut_SIZE(1);
    ASSERT_TLM_TickCount_SIZE(1);
    ASSERT_TLM_TickCount(0, 1);
}

}  // namespace Stm32
