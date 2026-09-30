// ======================================================================
// \title  Stm32UartDriverStub.cpp
// \author ivanlara
// \brief  HAL boundary stand-in for host-native unit tests. No USART1/DMA
//         hardware exists on the build host, so every call here operates
//         on injectable/observable stub state instead of real registers --
//         no HAL/CMSIS include, no register access. A unit test drives the
//         driver's state machine by calling
//         signalTxComplete()/signalRxChunk()/signalUartError() directly
//         (see Stm32UartDriver.hpp) instead of relying on a real ISR, and
//         controls/inspects the `extern` stub state below.
// ======================================================================

#include <fprime-stm32/Drv/STM32UartDriver/Stm32UartDriver.hpp>

// Injectable stub state for unit tests
extern bool Stub_hwOpenSucceeds = true;        // simulates MX_DMA_Init()/MX_USARTn_UART_Init()/initial RX arm
extern I32 Stub_hwOpenFailureStatus = 1;       // HAL_StatusTypeDef value reported when hwOpen fails (1 == HAL_ERROR)
extern Stm32::UsartInstance Stub_lastOpenedInstance =
    Stm32::UsartInstance::Usart1;              // instance most recently passed to hwOpen()

extern bool Stub_hwStartTxSucceeds = true;     // simulates HAL_UART_Transmit_DMA() acceptance
extern I32 Stub_hwStartTxFailureStatus = 1;    // HAL_StatusTypeDef value reported when hwStartTx fails

extern I32 Stub_hwRestartRxStatus = 0;         // HAL_StatusTypeDef value hwRestartRx() reports (0 == HAL_OK)

// Observable stub state for unit tests
extern U8 Stub_lastTxData[Stm32::Stm32UartDriverConfig::TX_STAGING_SIZE] = {0};
extern FwSizeType Stub_lastTxLen = 0;
extern U32 Stub_hwAbortTxCallCount = 0;
extern U32 Stub_hwAbortRxCallCount = 0;
extern U32 Stub_hwInvalidateRxStagingCallCount = 0;
extern U32 Stub_hwClearUartErrorCallCount = 0;

namespace {

// Mirrors stm32h7xx_hal_uart.h's HAL_UART_ERROR_* bit assignments
// (PE=0x01, NE=0x02, FE=0x04, ORE=0x08, DMA=0x10) so a unit test injecting
// an error code via signalUartError() sees the same classification a real
// HAL error would produce.
constexpr U32 RX_AFFECTING_ERROR_MASK = 0x01U | 0x02U | 0x04U | 0x08U;
constexpr U32 DMA_AFFECTING_ERROR_MASK = 0x10U;

}  // namespace

namespace Stm32 {

bool Stm32UartDriver ::hwOpen(UsartInstance instance, U32 preemptPriority, U32 subPriority, U32 requestedBaudRate,
                               U32& outActualBaudRate) {
    Stub_lastOpenedInstance = instance;

    if (!Stub_hwOpenSucceeds) {
        // Mirrors the real hwOpen(): the only HAL boundary method that
        // knows which call failed, so it emits HalError itself.
        Fw::LogStringArg _op("ReceiveToIdle_DMA");
        this->log_WARNING_HI_HalError(_op, Stub_hwOpenFailureStatus);
        return false;
    }
    outActualBaudRate = requestedBaudRate;  // no peripheral to configure; echo back what was requested
    return true;
}

bool Stm32UartDriver ::hwStartTx(const U8* data, FwSizeType len) {
    if (!Stub_hwStartTxSucceeds) {
        Fw::LogStringArg _op("Transmit_DMA");
        this->log_WARNING_HI_HalError(_op, Stub_hwStartTxFailureStatus);
        return false;
    }

    // Capture what was actually handed to the "DMA" so a unit test can
    // assert on the real bytes/length Stm32UartDriverCommon.cpp staged,
    // not just that hwStartTx returned true.
    const FwSizeType captured = (len < sizeof(Stub_lastTxData)) ? len : sizeof(Stub_lastTxData);
    for (FwSizeType i = 0; i < captured; i++) {
        Stub_lastTxData[i] = data[i];
    }
    Stub_lastTxLen = captured;
    return true;
}

void Stm32UartDriver ::hwAbortTx() {
    Stub_hwAbortTxCallCount++;
}

void Stm32UartDriver ::hwInvalidateRxStaging() {
    Stub_hwInvalidateRxStagingCallCount++;
}

I32 Stm32UartDriver ::hwRestartRx() {
    return Stub_hwRestartRxStatus;
}

void Stm32UartDriver ::hwAbortRx() {
    Stub_hwAbortRxCallCount++;
}

void Stm32UartDriver ::hwClearUartError() {
    Stub_hwClearUartErrorCallCount++;
}

void Stm32UartDriver ::hwClassifyUartError(U32 errorCode, bool& isRxAffecting, bool& isDmaAffecting) {
    isRxAffecting = (errorCode & RX_AFFECTING_ERROR_MASK) != 0U;
    isDmaAffecting = (errorCode & DMA_AFFECTING_ERROR_MASK) != 0U;
}

}  // namespace Stm32
