// ======================================================================
// \title  STM32Timer.hpp
// \author ivanlara
// \brief  hpp file for the STM32H7 hardware tick source (TIM2 CH2 output
//         compare), used to drive Svc::RateGroupDriver::CycleIn.
// ======================================================================

#ifndef STM32_TIMER_HPP
#define STM32_TIMER_HPP

#include <fprime-stm32/Drv/STM32Timer/STM32TimerComponentAc.hpp>

#include <Fw/Types/BasicTypes.hpp>
#include <config/Stm32Config.hpp>

namespace Stm32 {

//! TIM peripheral identifier, covering the general-purpose timers with a
//! channel 2 usable as this driver's output-compare tick source. Only Tim2
//! has a CubeMX-generated MX_TIMn_Init()/handle in this project today (see
//! stm32h753_hal/Core/Src/tim.c) -- the others are declared so a different
//! board/peripheral selection only requires adding that CubeMX config and
//! one switch case in STM32Timer.cpp, not touching this header, Common.cpp,
//! or the Stub.
enum class TimerInstance { Tim1, Tim2, Tim3, Tim4, Tim5, Tim6, Tim7, Tim8, Tim12, Tim13, Tim14, Tim15, Tim16, Tim17 };

class STM32Timer final : public STM32TimerComponentBase {
  public:
    //! Construct object STM32Timer
    explicit STM32Timer(const char* const compName);

    //! Destroy object STM32Timer
    ~STM32Timer();

    //! Arm the selected timer's channel 2 output-compare tick source. Must
    //! be called once from configureTopology(), after that timer's base
    //! counter has already been started (for Tim2, by Stm32_Tim2ClockInit()
    //! in Main.cpp; a different instance needs the equivalent project-level
    //! base-counter init/start call before this is called) -- this only
    //! configures and starts channel 2 on top of the already-running
    //! counter. It never calls MX_TIMn_Init() itself: re-running a CubeMX
    //! init function on an already-running timer risks glitching or
    //! resetting its counter, which would corrupt any other code (e.g.
    //! Os::RawTime) already relying on that counter running continuously.
    //! \param instance which TIM peripheral this driver instance owns; must
    //!        be enabled in Stm32Config.hpp (its `TIMn_INSTANCE` macro),
    //!        matching the peripheral selected in the project's `.ioc`
    //! \param periodUs: tick period in microseconds (the timer runs at
    //!        1 MHz, so this is a direct tick count with no conversion)
    void open(TimerInstance instance, U32 periodUs);

    //! Poll/drain step: consumes the ISR-latched tick flag, fires CycleOut
    //! with a fresh Os::RawTime timestamp, reprograms the next compare
    //! target, and detects/recovers a target that already elapsed before it
    //! could be reprogrammed (see TickOverrun). Called every cyclic-executive
    //! pass from Main.cpp, same pattern as Stm32::Stm32UartDriver::poll().
    void poll();

    //! Latch a pending tick. Called by the real ISR callback trampoline
    //! (HAL_TIM_OC_DelayElapsedCallback, a free function with no user-context
    //! pointer) on the stm32h7 target; a unit test may also call this
    //! directly to simulate a tick firing, since no ISR exists on the host.
    void signalTick();

  private:
    // ----------------------------------------------------------------------
    // HAL boundary: the only methods allowed to touch TIM_HandleTypeDef/HAL
    // calls or the raw TIM2 registers. Implemented once against the real
    // HAL in STM32Timer.cpp (stm32h7 target only) and once as a
    // fixed-behavior stand-in in STM32TimerStub.cpp (host unit tests).
    // ----------------------------------------------------------------------

    //! Resolve the selected TIM instance to its HAL handle and record it for
    //! every subsequent hwArmChannel()/hwReadCounter()/hwSetCompare() call.
    //! Must run before those -- hwReadCounter() is called (to compute the
    //! first compare target) before hwArmChannel() in open() below.
    void hwSelectInstance(TimerInstance instance);

    //! Configure the selected instance's channel 2 for output-compare
    //! "frozen" mode with the given initial compare target and start it in
    //! interrupt mode.
    void hwArmChannel(U32 target);

    //! Read the live free-running counter of the instance resolved by the
    //! most recent hwSelectInstance() call.
    U32 hwReadCounter();

    //! Reprogram channel 2's compare target on the already-armed channel.
    void hwSetCompare(U32 target);

    //! Which TIM peripheral this instance owns, re-resolved to a real HAL
    //! handle on every hw* call rather than cached in file-static state --
    //! this is what lets multiple STM32Timer instances (different physical
    //! timers) be open at the same time. The ISR callback trampoline
    //! (STM32Timer.cpp) uses a small fixed-size registry indexed by this
    //! same enum, since the HAL callback itself receives no
    //! instance/context, only a raw handle.
    TimerInstance m_instance;

    U32 m_periodTicks;   //!< tick period in TIM2 counts (1 count == 1 us)
    U32 m_nextTarget;    //!< CCR2 value currently armed
    U32 m_tickCount;     //!< total ticks raised, mirrors TickCount telemetry
    U32 m_overrunCount;  //!< total overruns recovered, mirrors OverrunCount telemetry
    bool m_opened;

    //! Latched by signalTick(); consumed by poll().
    volatile bool m_tickPending;

    //! Host-only fake TIM2 counter, advanced by hwReadCounter()/hwSetCompare()
    //! in STM32TimerStub.cpp so poll()'s overrun-detection logic is exercised
    //! by a unit test without real hardware. Unused on the stm32h7 target.
    U32 m_stubCounter;

    friend class STM32TimerTester;
};

}  // namespace Stm32

#endif