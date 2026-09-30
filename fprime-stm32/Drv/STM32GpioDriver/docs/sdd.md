# Stm32::Stm32GpioDriver

## 1. Introduction

`Stm32GpioDriver` is a bare-metal implementation of the [`Drv.Gpio`](../../../fprime/Drv/Interfaces/Gpio.fpp) interface for the STM32H7. Each instance owns exactly one GPIO line, configured once as either a push-pull output or a floating input via `open()`. Unlike [`Drv::LinuxGpioDriver`](../../../fprime/Drv/LinuxGpioDriver/docs/sdd.md), there is no character-device/ioctl layer and no interrupt-detection thread: `gpioWrite`/`gpioRead` are synchronous ports that call `HAL_GPIO_WritePin()`/`HAL_GPIO_ReadPin()` directly on the caller's thread, and `gpioInterrupt` is left unconnected (EXTI/NVIC-based interrupt support, if ever needed, belongs in a separate ISR-latch-and-poll design, not a polling thread — bare-metal execution here has none).

## 2. Requirements

| Name | Description | Validation |
|---|---|---|
| STM32-GPIO-COMP-001 | Shall implement the `Drv.Gpio` interface | inspection |
| STM32-GPIO-COMP-002 | Shall configure a GPIO line as a push-pull output or a floating input | inspection |
| STM32-GPIO-COMP-003 | Shall apply a caller-supplied default state before an output pin's mode is enabled, so it never glitches | inspection |
| STM32-GPIO-COMP-004 | Shall reject read/write requests that do not match the configured mode, and requests before `open()` | inspection |

## 3. Design

### 3.1 Port model

`import Gpio` provides `gpioWrite`/`gpioRead` (sync input) and `gpioInterrupt` (output, unconnected here). No events or telemetry: `open()` cannot fail (`HAL_GPIO_Init()` is `void`), and a mode mismatch is already communicated to the immediate, synchronous caller via the `Drv::GpioStatus` return value, so there is nothing left to log.

### 3.2 `open()`

Enables the target port's AHB4 clock (explicit `__HAL_RCC_GPIOx_CLK_ENABLE()` switch, matching the convention already used in `lib/fprime-stm32/src/usart.c`/`gpio.c`), writes the requested default level *before* switching an output pin's mode register, then calls `HAL_GPIO_Init()`. Ordering the write first avoids a transient glitch through whatever level happened to be latched in the output register at reset.

### 3.3 `gpioWrite`/`gpioRead`

Both check `m_opened` (`NOT_OPENED` if `open()` was never called) and `m_mode` (`INVALID_MODE` if the pin was configured for the other direction) before touching hardware, then call `HAL_GPIO_WritePin()`/`HAL_GPIO_ReadPin()` and translate `Fw::Logic::HIGH`/`LOW` to `GPIO_PIN_SET`/`RESET`.

## 4. Usage

```cpp
// configureTopology():
led1.open(GPIOF, GPIO_PIN_10, Drv::Stm32GpioDriver::GpioMode::OUTPUT, Fw::Logic::LOW);
```
