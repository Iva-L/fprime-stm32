# Stm32::STM32Timer

## 1. Introduction

`STM32Timer` is the periodic hardware tick source that drives the bare-metal cyclic executive's rate groups. It implements the [`Drv.Tick`](../../../../fprime/Drv/Interfaces/Tick.fpp) interface using a selectable TIM peripheral's **channel 2** in output-compare "frozen" mode (`TIM_OCMODE_TIMING`) — on TIM2, a second, independent channel layered on the *same* free-running TIM2 counter that `Os::RawTime` already uses (see `tim2_clock.cpp`). It never touches the selected timer's base counter, `ARR`, or the update/overflow interrupt any other code may depend on (e.g. `Os::RawTime` on TIM2), so the two coexist without interference.

Which TIM instance is used is selected per `open()` call (see §4.2) and must be enabled in [`Stm32Config.hpp`](../../config/Stm32Config.hpp) via its `TIMn_INSTANCE` macro, matching the peripheral actually configured in the project's `.ioc` — see §5 for the enable/select convention shared with `Stm32UartDriver`/`Stm32I2cDriver`.

This replaces software-polling `HAL_GetTick()` to detect elapsed milliseconds (the previous approach in `Main.cpp`) with a real hardware-interrupt-driven tick, matching the same ISR-latches-a-flag/poll-drains-it pattern already used by [`Stm32::Stm32UartDriver`](../../STM32UartDriver/docs/sdd.md).

## 2. Assumptions

Multiple `STM32Timer` instances may be added to a topology, one per physical TIM peripheral actually enabled in `Stm32Config.hpp`. This requires care in the real HAL boundary (`STM32Timer.cpp`), since the underlying ISR callback gives no instance context to work with:

- `HAL_TIM_OC_DelayElapsedCallback()` — the CMSIS/HAL ISR callback the shared `TIMn_IRQHandler` dispatches to — is a plain C free function with no user-context/opaque pointer parameter, so it has no way to know which `STM32Timer` C++ object's `signalTick()` to call, only the raw `TIM_HandleTypeDef*` the interrupt fired on.
- `hwSelectInstance()` (called from `open()`) resolves that instead by registering `{halHandle, this}` into a small fixed-size table (`s_timerRegistry`, sized to `TimerInstance`'s own cardinality and indexed by the instance itself) rather than a single file-static pointer. The ISR callback does a short linear scan of that table to find which live component owns the handle it was given, then forwards to that instance's `signalTick()`. This is the same registry-table pattern `Stm32::Stm32UartDriver` uses for its own HAL callbacks (`HAL_UART_TxCpltCallback`/etc.).
- Non-ISR HAL boundary methods (`hwArmChannel`/`hwReadCounter`/`hwSetCompare`) store only the HAL-free `TimerInstance` enum as a component member and re-resolve `toHalHandle(this->m_instance)` fresh on every call — no HAL pointer is cached in file-static state at all. Between the per-instance member and the registry table, two (or more) simultaneously-open `STM32Timer` instances share no mutable state, so opening a second instance against a different physical timer never disturbs the first's tick handling.
- Selecting a `TimerInstance` whose `TIMn_INSTANCE` macro is disabled in `Stm32Config.hpp` is treated the same way `Stm32UartDriver`/`Stm32I2cDriver` treat it: `FW_ASSERT` in `hwSelectInstance()` rather than dereferencing a null HAL handle, since it is a build-time configuration mistake (a topology asking for a peripheral the `.ioc`/config never enabled), not a runtime condition to tolerate.
- This driver never calls `HAL_NVIC_EnableIRQ`/`HAL_NVIC_SetPriority` itself (unlike `Stm32UartDriver`, which parameterizes NVIC priority per instance in `hwOpen()`) — a second physical timer's NVIC interrupt must already be enabled by that timer's own CubeMX-generated init/MSP code (the same project-level responsibility already noted for the base-counter start in §4.2), not by this component.

## 3. Requirements

| Name | Description | Validation |
|---|---|---|
| STM32-TIMER-COMP-001 | Shall implement the `Drv.Tick` interface | inspection |
| STM32-TIMER-COMP-002 | Shall raise ticks from a TIM2 hardware interrupt, not software polling of another clock | inspection |
| STM32-TIMER-COMP-003 | Shall not disturb TIM2's base counter or the microsecond clock built on it | inspection |
| STM32-TIMER-COMP-004 | Shall detect and recover a compare target that elapses before it can be reprogrammed, without stalling until TIM2's ~71-minute wraparound | test |
| STM32-TIMER-COMP-005 | Shall never perform F´ port calls or allocation from ISR context | inspection |

## 4. Design

### 4.1 Port model

`import Drv.Tick` provides the single `CycleOut: Svc.Cycle` output port, wired in `topology.fpp` to `rateGroupDriver.CycleIn`. There are no input ports to implement — nothing needs to call into this component from the port graph; `open()` and `poll()` are plain public methods called directly, the same pattern used by `Stm32UartDriver::open()`/`poll()`.

### 4.2 `open(instance, periodUs)`

Called once from `configureTopology()`, after the selected instance's base counter has already been started (for `TimerInstance::Tim2`, by `Stm32_Tim2ClockInit()` in `Main.cpp`; a different instance needs the equivalent project-level init/start call before `open()` runs — `open()` never calls `MX_TIMn_Init()` itself, since re-running a CubeMX init function on an already-running timer risks glitching or resetting its counter). `hwSelectInstance()` resolves `instance` to its HAL handle (asserting if that instance's `TIMn_INSTANCE` macro is disabled), then `open()` arms that timer's channel 2 via `HAL_TIM_OC_ConfigChannel()` (mode `TIM_OCMODE_TIMING`, so the channel has no effect on any GPIO pin) and `HAL_TIM_OC_Start_IT()`, with the first compare target set to the live counter value plus `periodUs` (the timer runs at 1 MHz, so 1 tick == 1 microsecond — no unit conversion needed). Emits `Configured` once on success.

### 4.3 `poll()`

Called every cyclic-executive pass from `Main.cpp` (not through a rate group — a 1 Hz/0.5 Hz/0.25 Hz cadence would be far too slow to service a 10 ms tick). `HAL_TIM_OC_DelayElapsedCallback()` (the ISR, dispatched by the existing shared `TIM2_IRQHandler`) only latches a `volatile` pending flag; `poll()` is where the real work happens:

1. If the flag is set: clear it, increment `TickCount`, and fire `CycleOut_out()` with a fresh `Os::RawTime` timestamp — this is the deterministic, non-ISR context where F´ port calls belong.
2. Reschedule the next compare target *relative to the target that just fired* (`m_nextTarget + periodTicks`), not relative to "now" — this keeps ticks periodic against the original schedule instead of drifting by however late `poll()` happened to run.
3. Before committing that new target, check whether `TIM2->CNT` has already passed it (a signed-difference comparison, correct across TIM2's 32-bit wraparound as long as the gap being measured is far smaller than half that span, which any real overrun is). If so, the gap between `poll()` calls was longer than one tick period — the compare would otherwise sit un-armed until the counter wraps around and reaches that value again, roughly 71 minutes later. Instead: increment `OverrunCount`, emit a throttled `TickOverrun` event, and resync the target to `now + periodTicks`.

## 5. Usage

```cpp
// configureTopology():
timer.open(Stm32::TimerInstance::Tim2, 10000);  // 10 ms period, matching rateGroupDivisorsSet's {100, 200, 400}

// Main.cpp cyclic executive loop, every pass:
ReferenceDeployment::timer.poll();
```

To use a different TIM instance: enable it in `Stm32Config.hpp` (its
`TIMn_INSTANCE` macro), regenerate the CubeMX project with that peripheral
configured, provide a base-counter init/start call for it (following
`Stm32_Tim2ClockInit()`'s pattern in `tim2_clock.cpp`) called before `open()`,
then pass the matching `Stm32::TimerInstance` value to `open()`. This is the
same enable-in-config/select-in-`open()` convention used by
`Stm32UartDriver`/`Stm32I2cDriver` for their own peripheral instances.

## 6. Events and telemetry

Events: `Configured`, `TickOverrun`. Telemetry: `TickCount`, `OverrunCount`.
