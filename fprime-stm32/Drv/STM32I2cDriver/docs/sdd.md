# Stm32::Stm32I2cDriver

## 1. Introduction

`Stm32I2cDriver` is a bare-metal implementation of the [`Drv.I2c`](../../../fprime/Drv/Interfaces/I2c.fpp) interface for an STM32H7 I2C peripheral, selected per instance at `open()` time. It is a raw bus-master driver only: `write`/`read`/`writeRead` map onto a synchronous master transaction against whatever 7-bit address the caller supplies -- either blocking `HAL_I2C_Master_Transmit()`/`HAL_I2C_Master_Receive()` calls (`Stm32::TransferMode::POLLED`, the default) or their `_DMA` counterparts, which the handler blocks on until completion or timeout (`Stm32::TransferMode::DMA`), selected per-instance at `open()` time. There is no sensor-specific register model layered on top -- a sensor component (or a topology-level test command) is expected to drive this component's ports directly with the sensor's own register addressing convention.

## 2. Requirements

| Name | Description | Validation |
|---|---|---|
| STM32-I2C-COMP-001 | Shall implement the `Drv.I2c` interface | inspection |
| STM32-I2C-COMP-002 | Shall configure a caller-selected I2C instance (via `MX_I2Cn_Init()`) at a caller-selected bus speed preset, and reject transactions issued before `open()` | inspection |
| STM32-I2C-COMP-003 | Shall perform a write to a 7-bit slave address, bounded by a 10 ms transaction watchdog, using either a blocking call (POLLED) or a DMA transfer the caller blocks on (DMA), selected per instance at `open()` | inspection, test |
| STM32-I2C-COMP-004 | Shall perform a read from a 7-bit slave address, bounded by a 10 ms transaction watchdog, in either POLLED or DMA mode | inspection, test |
| STM32-I2C-COMP-005 | Shall perform a write followed by a read for the same address without requiring a second port invocation | inspection |
| STM32-I2C-COMP-006 | Shall distinguish an address-phase NACK (`I2C_ADDRESS_ERR`) from a data-phase/bus failure (`I2C_WRITE_ERR`/`I2C_READ_ERR`) | inspection |
| STM32-I2C-COMP-007 | A DMA-mode transaction shall clean the D-cache over a write buffer before, and invalidate it over a read buffer after, the transfer, and shall assert both buffers are reachable by the DMA controller (not DTCM) | inspection |

## 3. Design

### 3.1 Port model

`import Drv.I2c` provides `write`/`read`/`writeRead` as guarded, synchronous input ports; each returns a `Drv::I2cStatus` to the caller directly, so there is no completion port and no telemetry. The only event is `HalError`, emitted from the HAL boundary whenever a blocking `HAL_I2C_*` call does not return `HAL_OK`.

### 3.2 Common/Real/Stub split

Following the convention in `lib/fprime-stm32/README.md`, this driver splits into three files sharing one HAL-free header (`Stm32I2cDriver.hpp`):

- `Stm32I2cDriverCommon.cpp` -- port handler logic and `open()`, all hardware-independent. Always built.
- `Stm32I2cDriver.cpp` -- the real HAL boundary (`hwOpen`/`hwMasterTransmit`/`hwMasterReceive`/`hwIsAddressNack`), built only for the `stm32h7` target. The only file allowed to include `i2c.h`/call `HAL_I2C_*`.
- `Stm32I2cDriverStub.cpp` -- the same boundary methods against injectable/observable stub state, built for host UT and any non-`stm32h7` build. Never included in a flight build.

### 3.3 `open(instance, busSpeed, mode)`

`open()` takes an `I2cInstance` (`I2c1`/`I2c2`/`I2c3`/`I2c4`, mirroring `Stm32UartDriver`'s `UsartInstance` switch), an `I2cBusSpeed` preset (`Standard`/`Fast`, defaulting to `Fast` to match this project's current configuration), and a `Stm32::TransferMode` (default `POLLED`, trailing both other params so every existing call site keeps compiling and keeps its current, polled behavior unchanged). It calls the CubeMX-generated `MX_I2Cn_Init()` for the selected instance (I2C1/GPIO/clock configuration in `lib/fprime-stm32/src/i2c.c`), then applies the requested speed by overwriting `Init.Timing` and re-running `HAL_I2C_Init()` if it differs from what `MX_I2Cn_Init()` just applied.

When `mode` is `DMA`, `open()` additionally calls `MX_DMA_Init()`, asserts that CubeMX has actually linked a DMA stream to this instance (`halHandle->hdmatx`/`hdmarx` non-null -- see the warning in §3.5), and registers `{halHandle, this}` into a small fixed-size table (`s_i2cRegistry`, sized to `I2cInstance`'s cardinality) so `HAL_I2C_MasterTxCpltCallback()`/`HAL_I2C_MasterRxCpltCallback()`/`HAL_I2C_ErrorCallback()` -- plain HAL callbacks with no instance/context argument -- can route back to the correct component instance, the same registry pattern `Stm32UartDriver` uses for its own ISR callbacks (see its `docs/sdd.md` §3.7). A POLLED instance is never registered.

Only `I2c1` has a CubeMX-generated handle/init function in this project today -- selecting `I2c2`/`I2c3`/`I2c4` asserts in the real HAL boundary (`Stm32I2cDriver.cpp`'s `toHalHandle()`/`callInstanceInit()`). Adding a second instance for a different board only requires generating that peripheral's CubeMX config and one switch case in each of those two functions; the header, `Common.cpp`, and the `Stub` are already instance-agnostic.

Unlike UART's `BaudRate` (computed at runtime by `HAL_UART_Init()` from the peripheral clock), the STM32H7 I2C `Timing` register has no closed-form runtime formula in the HAL. `I2cBusSpeed`'s two presets map to two CubeMX-computed constants for this project's actual D2PCLK1 clock (`0x00B03FDB` for Fast, `0x307075B1` for Standard -- the latter was CubeMX's original preset before the "Increased I2C clock speed to 400 kHz" commit raised it). A third preset can only be added once CubeMX has actually generated its Timing value for this clock tree -- do not hand-derive one.

`MX_I2Cn_Init()` traps in `Error_Handler()` on failure rather than returning a status, matching every other `MX_*_Init()` in this project, so `open()` cannot observe a failure there -- only the speed-override `HAL_I2C_Init()` call (when the requested preset differs from CubeMX's default) has a real failure path.

**Multi-instance safety**: `open()` stores only the HAL-free `I2cInstance` enum and the requested `TransferMode` as component members; `hwMasterTransmit`/`hwMasterReceive` re-resolve `toHalHandle(this->m_instance)` fresh on every call, rather than caching the resolved `I2C_HandleTypeDef*` in file-static state. This is what lets two `Stm32I2cDriver` instances -- e.g. one for an IMU on I2C1 and a second for a temperature sensor on I2C2 -- be open at the same time, independently POLLED or DMA, with no shared mutable state between them.

### 3.4 `write`/`read`/`writeRead`

Each handler asserts the caller's `Fw::Buffer`(s) are non-null and fit in a `U16` (the HAL's `Size`/`DevAddress` parameter width), then calls the corresponding `hw*` boundary method -- unchanged by `TransferMode`; the `POLLED`/`DMA` branch lives entirely inside `hwMasterTransmit()`/`hwMasterReceive()` (§3.5). On failure, `HAL_I2C_GetError() & HAL_I2C_ERROR_AF` distinguishes "no device answered" (`I2C_ADDRESS_ERR`) from a data-phase/bus failure (`I2C_WRITE_ERR`/`I2C_READ_ERR`).

`writeRead` issues a transmit immediately followed by a receive to the same address -- a STOP and a fresh START, not a single electrically-held repeated START, in either mode. Because this bus has exactly one master, a STOP-then-START is safe for the common case (nothing else can interrupt the sequence); a sensor that strictly requires the bus held across the register-address write (loses its internal pointer on STOP) will not work correctly until this is upgraded.

### 3.5 `hwMasterTransmit()`/`hwMasterReceive()` and the DMA path

Both branch on `m_transferMode`:

- **POLLED**: a single blocking `HAL_I2C_Master_Transmit()`/`HAL_I2C_Master_Receive()` call bounded by `TRANSACTION_TIMEOUT_MS`.
- **DMA**: `Stm32::AssertDmaSafe()` on the buffer (it must not overlap DTCM), `Stm32::CleanDCacheForDma()` on a write, `HAL_I2C_Master_Transmit_DMA()`/`HAL_I2C_Master_Receive_DMA()`, then a bounded busy-wait (an `Os::RawTime`-timed loop, capped by `TRANSACTION_TIMEOUT_MS`) on a `volatile` flag that `HAL_I2C_MasterTxCpltCallback()`/`HAL_I2C_MasterRxCpltCallback()`/`HAL_I2C_ErrorCallback()` clear from the ISR. On timeout, `HAL_I2C_Master_Abort_IT()` runs and the caller sees the same error reporting as any other HAL failure. On a successful receive, `Stm32::InvalidateDCacheForDma()` runs on the read buffer before returning.

**Known limitation**: this project's current CubeMX configuration for I2C1 has no DMA request attached (`lib/fprime-stm32/src/i2c.c`'s `HAL_I2C_MspInit()` never calls `__HAL_LINKDMA()`), so `open(..., TransferMode::DMA)` on I2C1 will fail its `hdmatx`/`hdmarx` assertion today rather than silently dereferencing a null DMA handle during a transaction. Using DMA mode on real hardware requires regenerating the CubeMX project with a DMA request added to the target I2C instance first -- the same prerequisite §3.3 already documents for enabling an instance beyond I2C1 at all.

## 4. Usage

```cpp
// configureTopology(): POLLED is the default -- unchanged from before TransferMode existed.
i2cDriver.open(Stm32::I2cInstance::I2c1, Stm32::I2cBusSpeed::Fast);

// raw sensor probe/register read, e.g. from a test command or adapter component:
Drv::I2cStatus status = i2cDriver.get_write_InputPort(0)->invoke(sensorAddr, regAddrBuffer);
status = i2cDriver.get_writeRead_InputPort(0)->invoke(sensorAddr, regAddrBuffer, readDataBuffer);
```

Note that the caller owns the buffers passed to each port; the driver does not retain or deallocate them.

To use a different I2C instance: enable it in `Stm32Config.hpp` (its `I2Cn_Instance` macro), regenerate the CubeMX project with that peripheral configured, call `open()`, then pass the matching `Stm32::I2cInstance` value to `open()` as well as the matching `Stm32::I2cBusSpeed`.