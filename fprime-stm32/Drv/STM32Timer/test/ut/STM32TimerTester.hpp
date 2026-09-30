// ======================================================================
// \title  STM32TimerTester.hpp
// \author ivanlara
// \brief  hpp file for STM32Timer component test harness implementation class
// ======================================================================

#ifndef Stm32_STM32TimerTester_HPP
#define Stm32_STM32TimerTester_HPP

#include "fprime-stm32/Drv/STM32Timer/STM32Timer.hpp"
#include "fprime-stm32/Drv/STM32Timer/STM32TimerGTestBase.hpp"

namespace Stm32 {

class STM32TimerTester final : public STM32TimerGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 10;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object STM32TimerTester
    STM32TimerTester();

    //! Destroy object STM32TimerTester
    ~STM32TimerTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! open() arms the channel and emits Configured
    void testOpenArms();

    //! poll() with no tick pending raises nothing
    void testPollWithoutTick();

    //! signalTick() + poll() raises CycleOut and TickCount telemetry
    void testPollWithTick();

    //! A compare target that has already elapsed is detected as an overrun
    void testOverrunDetection();

    //! now == nextTarget exactly (zero elapsed) still counts as an overrun,
    //! per the real driver's `>= 0` signed-difference check
    void testPollWithTickNoOverrunAtExactBoundary();

    //! Two simultaneously-open STM32Timer instances (different physical
    //! timers) don't share any mutable state: opening a second instance
    //! after the first is already open must not disturb the first
    //! instance's own tick handling. Before the multi-instance fix, the
    //! real HAL boundary's hwSelectInstance() would have hard FW_ASSERT'd
    //! on this second open() outright.
    void testTwoInstancesDoNotInterfere();

  private:
    // ----------------------------------------------------------------------
    // Test support
    // ----------------------------------------------------------------------

    //! Reset every Stub_* global back to its documented default so each
    //! test starts from a known, hermetic state regardless of run order --
    //! these are shared, process-wide globals (see STM32TimerStub.cpp), not
    //! per-Tester state.
    void resetStubState();

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    STM32Timer component;
};

}  // namespace Stm32

#endif
