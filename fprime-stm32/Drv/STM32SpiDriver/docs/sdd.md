# Stm32::Stm32SpiDriver

## 1. Introduction

`Stm32SpiDriver` is a bare-metal implementation of the [`Drv.Spi`](../../../fprime/Drv/Interfaces/Spi.fpp) interface for an STM32H7 SPI peripheral, selected per instance at `open()` time. It is a raw bus-master driver only: `SpiWriteRead`/`SpiReadWrite` map onto a single, synchronous full-duplex transfer bracketed by a caller-selected chip-select GPIO pin the driver drives itself -- either a blocking `HAL_SPI_TransmitReceive()` call (`Stm32::TransferMode::POLLED`, the default) or a `HAL_SPI_TransmitReceive_DMA()` call that the handler blocks on until completion or timeout (`Stm32::TransferMode::DMA`), selected per-instance at `open()` time (see §3.3/§3.5). There is no sensor-specific register model layered on top -- a sensor component (or a topology-level test command) is expected to drive this component's ports directly, framing its own register-address/data bytes into the write buffer.

## 2. Requirements

| Name | Description | Validation |
|---|---|---|
| STM32-SPI-COMP-001 | Shall implement the `Drv.Spi` interface | inspection |
| STM32-SPI-COMP-002 | Shall configure a caller-selected SPI instance (via `MX_SPIn_Init()`) and a caller-selected chip-select GPIO pin at `open()`, and reject transactions issued before `open()` | inspection |
| STM32-SPI-COMP-003 | Shall drive the configured chip-select pin low for the duration of each transfer and high otherwise | inspection |
| STM32-SPI-COMP-004 | Shall perform a blocking full-duplex transfer of equal-sized write/read buffers, bounded by a caller-selected transaction watchdog | inspection |
| STM32-SPI-COMP-005 | Shall support multiple simultaneous instances (different SPI peripheral and/or chip-select pin per instance, independently POLLED or DMA) with no shared mutable HAL state between them | inspection |
| STM32-SPI-COMP-006 | Shall select POLLED (default) or DMA transfer per instance at `open()` time; a POLLED instance uses a single blocking `HAL_SPI_TransmitReceive()` call, a DMA instance uses `HAL_SPI_TransmitReceive_DMA()` and blocks the caller until completion or timeout | inspection, test |
| STM32-SPI-COMP-007 | A DMA-mode transfer shall clean the D-cache over the write buffer before, and invalidate it over the read buffer after, the transfer, and shall assert both buffers are reachable by the DMA controller (not DTCM) | inspection |

## 3. Design

### 3.1 Port model

`import Drv.Spi` provides `SpiWriteRead` (guarded, returns a `Drv::SpiStatus`) and the deprecated `SpiReadWrite` (plain synchronous, no return value -- forwards to `SpiWriteRead` and discards the status, matching `Drv::LinuxSpiDriver`'s own handling of this port). Both ports take the same `ref writeBuffer: Fw.Buffer, ref readBuffer: Fw.Buffer` pair; the two buffers must be the same size, since a SPI transfer is inherently full-duplex (one byte clocked out for every byte clocked in). The only event is `HalError`, emitted from the HAL boundary whenever the blocking `HAL_SPI_TransmitReceive()` call does not return `HAL_OK`.

### 3.2 Common/Real/Stub split

Following the convention in `lib/fprime-stm32/README.md`, this driver splits into three files sharing one HAL-free header (`Stm32SpiDriver.hpp`):

- `Stm32SpiDriverCommon.cpp` -- port handler logic, all hardware-independent. Always built.
- `Stm32SpiDriver.cpp` -- the real HAL boundary (`open`/`hwSetCs`/`hwTransmitReceive`), built only for the `stm32h7` target. The only file allowed to include `spi.h`/call `HAL_SPI_*`/`HAL_GPIO_*`.
- `Stm32SpiDriverStub.cpp` -- the same boundary methods against injectable/observable stub state, built for host UT and any non-`stm32h7` build. Never included in a flight build.

### 3.3 `open(instance, csPort, csPin, timeoutMs, mode)`

`open()` takes an `SpiInstance` (`Spi1`-`Spi6`, mirroring `Stm32I2cDriver`'s `I2cInstance` switch), a chip-select GPIO identifier (`Stm32::GpioPort` + pin -- the same vocabulary `Stm32GpioDriver::open()` uses, so no new "which pin" type is invented), a per-transaction timeout in milliseconds (default 10 ms, matching `Stm32I2cDriver`'s watchdog convention), and a `Stm32::TransferMode` (default `POLLED`, trailing both other params so every existing call site keeps compiling and keeps its current, polled behavior unchanged). It calls the CubeMX-generated `MX_SPIn_Init()` for the selected instance, then configures the chip-select pin as a push-pull output, idle HIGH (deselected) -- SPI5 is configured `NSS_SOFT` in this project's CubeMX setup, so the HAL never drives any CS pin on its own; this driver owns that GPIO directly, the same way it owns the SPI peripheral. When `mode` is `DMA`, `open()` additionally registers `{halHandle, this}` into a small fixed-size table (`s_spiRegistry`, sized to `SpiInstance`'s cardinality) so `HAL_SPI_TxRxCpltCallback()`/`HAL_SPI_ErrorCallback()` -- plain HAL callbacks with no instance/context argument -- can route back to the correct component instance, the same registry pattern `Stm32UartDriver` uses for its own ISR callbacks (see its `docs/sdd.md` §3.7). A POLLED instance is never registered.

Only `Spi5` has a CubeMX-generated handle/init function in this project today -- selecting another instance asserts in the real HAL boundary (`Stm32SpiDriver.cpp`'s `toHalHandle()`/`callInstanceInit()`). Adding a second instance for a different board only requires generating that peripheral's CubeMX config and one switch case in each of those two functions; the header, `Common.cpp`, and the `Stub` are already instance-agnostic.

**Multi-instance safety**: this driver stores only HAL-free state on the component itself (the `SpiInstance` enum, the `GpioPort`+pin, the timeout) and re-resolves the actual HAL pointers from that state on every `hwSetCs()`/`hwTransmitReceive()` call, matching the same per-instance approach every STM32 driver in this library now uses. This is deliberate: a BMP280 and any future SPI sensor on a different bus or chip-select need two simultaneously-open `Stm32SpiDriver` instances that never share mutable state.

`MX_SPIn_Init()` traps in `Error_Handler()` on failure rather than returning a status, matching every other `MX_*_Init()` in this project, so `open()` cannot observe a failure there.

### 3.4 `SpiWriteRead`/`SpiReadWrite`

The handler (`Stm32SpiDriverCommon.cpp`) asserts both of the caller's `Fw::Buffer`s are non-null and the same size, drives chip-select low, calls `hwTransmitReceive()`, and drives chip-select high again -- unconditionally, even on a HAL failure, so a failed transfer never leaves the device selected. This handler is mode-agnostic; the `POLLED`/`DMA` branch lives entirely inside `hwTransmitReceive()` (the HAL boundary, §3.5), not here.

### 3.5 `hwTransmitReceive()` and the DMA path

`hwTransmitReceive()` branches on `m_transferMode`:

- **POLLED**: a single blocking `HAL_SPI_TransmitReceive()` call bounded by the `open()`-time timeout. Sufficient on its own for small sensor payloads (6-8 bytes for the BMP280) -- no DMA setup, no AXI SRAM buffer-alignment requirement, no D-cache coherence concern.
- **DMA**: `Stm32::AssertDmaSafe()` on both buffers (they must not overlap DTCM, which no DMA controller on this chip can reach), `Stm32::CleanDCacheForDma()` on the write buffer, `HAL_SPI_TransmitReceive_DMA()`, then a bounded busy-wait (an `Os::RawTime`-timed loop, capped by the same `open()`-time timeout) on a `volatile` flag that `HAL_SPI_TxRxCpltCallback()`/`HAL_SPI_ErrorCallback()` clear from the ISR. On timeout, `HAL_SPI_Abort()` runs and the caller sees the same `SPI_WRITE_ERR`/`HalError` reporting as any other HAL failure. On completion, `Stm32::InvalidateDCacheForDma()` runs on the read buffer before returning. This keeps `SpiWriteRead`'s synchronous port contract identical in both modes -- the caller cannot observe which mode is in effect except through timing.

**Known limitation**: this project's current CubeMX configuration for SPI5 has no DMA request attached (`lib/fprime-stm32/src/spi.c`'s `HAL_SPI_MspInit()` never calls `__HAL_LINKDMA()`), so `open(..., TransferMode::DMA)` on Spi5 will fail an `hdmatx`/`hdmarx` assertion in `open()` today rather than silently dereferencing a null DMA handle during a transfer. Using DMA mode on real hardware (e.g. for bulk flash access on SPI1) requires regenerating the CubeMX project with a DMA request added to that instance first -- the same prerequisite §3.3 already documents for enabling an instance beyond SPI5 at all.

## 4. Usage

```cpp
// configureTopology(): POLLED is the default -- unchanged from before TransferMode existed.
spiDriver.open(Stm32::SpiInstance::Spi5, Stm32::GpioPort::F, /* csPin */ 6);

// A second instance on a different bus, e.g. bulk flash access on SPI1,
// opted into DMA explicitly:
flashSpiDriver.open(Stm32::SpiInstance::Spi1, Stm32::GpioPort::A, /* csPin */ 4,
                     /* timeoutMs */ 50, Stm32::TransferMode::DMA);

// raw sensor register read, e.g. from a test command or adapter component:
Fw::Buffer writeBuffer(regAddrAndDummyBytes, size);
Fw::Buffer readBuffer(responseBytes, size);
Drv::SpiStatus status = spiDriver.get_SpiWriteRead_InputPort(0)->invoke(writeBuffer, readBuffer);
```

Note that the caller owns the buffers passed to each port; the driver does not retain or deallocate them, and `writeBuffer`/`readBuffer` must be the same size (SPI's full-duplex contract -- one byte out for every byte in).

To use a different SPI instance: enable it in `Stm32Config.hpp` (its `SPIn_INSTANCE` macro), regenerate the CubeMX project with that peripheral configured, then pass the matching `Stm32::SpiInstance` value to `open()` along with that instance's own chip-select pin.
