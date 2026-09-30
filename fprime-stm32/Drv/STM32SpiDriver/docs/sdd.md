# Stm32::Stm32SpiDriver

## 1. Introduction

`Stm32SpiDriver` is a bare-metal implementation of the [`Drv.Spi`](../../../fprime/Drv/Interfaces/Spi.fpp) interface for an STM32H7 SPI peripheral, selected per instance at `open()` time. It is a raw bus-master driver only: `SpiWriteRead`/`SpiReadWrite` map directly onto a single blocking (polled) `HAL_SPI_TransmitReceive()` call, bracketed by a caller-selected chip-select GPIO pin the driver drives itself. There is no sensor-specific register model layered on top -- a sensor component (or a topology-level test command) is expected to drive this component's ports directly, framing its own register-address/data bytes into the write buffer.

## 2. Requirements

| Name | Description | Validation |
|---|---|---|
| STM32-SPI-COMP-001 | Shall implement the `Drv.Spi` interface | inspection |
| STM32-SPI-COMP-002 | Shall configure a caller-selected SPI instance (via `MX_SPIn_Init()`) and a caller-selected chip-select GPIO pin at `open()`, and reject transactions issued before `open()` | inspection |
| STM32-SPI-COMP-003 | Shall drive the configured chip-select pin low for the duration of each transfer and high otherwise | inspection |
| STM32-SPI-COMP-004 | Shall perform a blocking full-duplex transfer of equal-sized write/read buffers, bounded by a caller-selected transaction watchdog | inspection |
| STM32-SPI-COMP-005 | Shall support multiple simultaneous instances (different SPI peripheral and/or chip-select pin per instance) with no shared mutable HAL state between them | inspection |
| STM32-SPI-COMP-006 | Shall not use DMA or interrupts -- every transfer is a single polled `HAL_SPI_TransmitReceive()` call | inspection |

## 3. Design

### 3.1 Port model

`import Drv.Spi` provides `SpiWriteRead` (guarded, returns a `Drv::SpiStatus`) and the deprecated `SpiReadWrite` (plain synchronous, no return value -- forwards to `SpiWriteRead` and discards the status, matching `Drv::LinuxSpiDriver`'s own handling of this port). Both ports take the same `ref writeBuffer: Fw.Buffer, ref readBuffer: Fw.Buffer` pair; the two buffers must be the same size, since a SPI transfer is inherently full-duplex (one byte clocked out for every byte clocked in). The only event is `HalError`, emitted from the HAL boundary whenever the blocking `HAL_SPI_TransmitReceive()` call does not return `HAL_OK`.

### 3.2 Common/Real/Stub split

Following the convention in `lib/fprime-stm32/README.md`, this driver splits into three files sharing one HAL-free header (`Stm32SpiDriver.hpp`):

- `Stm32SpiDriverCommon.cpp` -- port handler logic, all hardware-independent. Always built.
- `Stm32SpiDriver.cpp` -- the real HAL boundary (`open`/`hwSetCs`/`hwTransmitReceive`), built only for the `stm32h7` target. The only file allowed to include `spi.h`/call `HAL_SPI_*`/`HAL_GPIO_*`.
- `Stm32SpiDriverStub.cpp` -- the same boundary methods against injectable/observable stub state, built for host UT and any non-`stm32h7` build. Never included in a flight build.

### 3.3 `open(instance, csPort, csPin, timeoutMs)`

`open()` takes an `SpiInstance` (`Spi1`-`Spi6`, mirroring `Stm32I2cDriver`'s `I2cInstance` switch), a chip-select GPIO identifier (`Stm32::GpioPort` + pin -- the same vocabulary `Stm32GpioDriver::open()` uses, so no new "which pin" type is invented), and a per-transaction timeout in milliseconds (default 10 ms, matching `Stm32I2cDriver`'s watchdog convention). It calls the CubeMX-generated `MX_SPIn_Init()` for the selected instance, then configures the chip-select pin as a push-pull output, idle HIGH (deselected) -- SPI5 is configured `NSS_SOFT` in this project's CubeMX setup, so the HAL never drives any CS pin on its own; this driver owns that GPIO directly, the same way it owns the SPI peripheral.

Only `Spi5` has a CubeMX-generated handle/init function in this project today -- selecting another instance asserts in the real HAL boundary (`Stm32SpiDriver.cpp`'s `toHalHandle()`/`callInstanceInit()`). Adding a second instance for a different board only requires generating that peripheral's CubeMX config and one switch case in each of those two functions; the header, `Common.cpp`, and the `Stub` are already instance-agnostic.

**Multi-instance safety**: this driver stores only HAL-free state on the component itself (the `SpiInstance` enum, the `GpioPort`+pin, the timeout) and re-resolves the actual HAL pointers from that state on every `hwSetCs()`/`hwTransmitReceive()` call, matching the same per-instance approach every STM32 driver in this library now uses. This is deliberate: a BMP280 and any future SPI sensor on a different bus or chip-select need two simultaneously-open `Stm32SpiDriver` instances that never share mutable state.

`MX_SPIn_Init()` traps in `Error_Handler()` on failure rather than returning a status, matching every other `MX_*_Init()` in this project, so `open()` cannot observe a failure there.

### 3.4 `SpiWriteRead`/`SpiReadWrite`

The handler asserts both of the caller's `Fw::Buffer`s are non-null and the same size, drives chip-select low, performs the single blocking `HAL_SPI_TransmitReceive()` call (bounded by the `open()`-time timeout), and drives chip-select high again -- unconditionally, even on a HAL failure, so a failed transfer never leaves the device selected. Small sensor payloads (6-8 bytes for the BMP280) make a single polled call sufficient; there is no DMA setup, no AXI SRAM buffer-alignment requirement, and no Cortex-M7 D-cache coherence concern to manage (contrast `Stm32UartDriver`, which does need all of that for its larger, continuous ground-link traffic).

## 4. Usage

```cpp
// configureTopology():
spiDriver.open(Stm32::SpiInstance::Spi5, Stm32::GpioPort::F, /* csPin */ 6);

// raw sensor register read, e.g. from a test command or adapter component:
Fw::Buffer writeBuffer(regAddrAndDummyBytes, size);
Fw::Buffer readBuffer(responseBytes, size);
Drv::SpiStatus status = spiDriver.get_SpiWriteRead_InputPort(0)->invoke(writeBuffer, readBuffer);
```

Note that the caller owns the buffers passed to each port; the driver does not retain or deallocate them, and `writeBuffer`/`readBuffer` must be the same size (SPI's full-duplex contract -- one byte out for every byte in).

To use a different SPI instance: enable it in `Stm32Config.hpp` (its `SPIn_INSTANCE` macro), regenerate the CubeMX project with that peripheral configured, then pass the matching `Stm32::SpiInstance` value to `open()` along with that instance's own chip-select pin.
