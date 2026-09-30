// ======================================================================
// \title  Stm32UartDriverTester.cpp
// \author ivanlara
// \brief  cpp file for Stm32UartDriver component test harness implementation class
// ======================================================================

#include "Stm32UartDriverTester.hpp"

#include <Fw/Time/TimeInterval.hpp>
#include <Os/Task.hpp>

// Stub_* globals defined in Stm32UartDriverStub.cpp -- declared here so this
// Tester can inject HAL boundary success/failure and inspect exactly what
// Stm32UartDriverCommon.cpp drove through it, instead of only exercising the
// stub's fixed defaults.
extern bool Stub_hwOpenSucceeds;
extern I32 Stub_hwOpenFailureStatus;
extern Stm32::UsartInstance Stub_lastOpenedInstance;
extern bool Stub_hwStartTxSucceeds;
extern I32 Stub_hwStartTxFailureStatus;
extern I32 Stub_hwRestartRxStatus;
extern U8 Stub_lastTxData[Stm32::Stm32UartDriverConfig::TX_STAGING_SIZE];
extern FwSizeType Stub_lastTxLen;
extern U32 Stub_hwAbortTxCallCount;
extern U32 Stub_hwAbortRxCallCount;
extern U32 Stub_hwInvalidateRxStagingCallCount;
extern U32 Stub_hwClearUartErrorCallCount;

namespace Stm32 {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

Stm32UartDriverTester ::Stm32UartDriverTester()
    : Stm32UartDriverGTestBase("Stm32UartDriverTester", Stm32UartDriverTester::MAX_HISTORY_SIZE),
      component("Stm32UartDriver") {
    this->resetStubState();
    this->initComponents();
    this->connectPorts();
}

Stm32UartDriverTester ::~Stm32UartDriverTester() {
    this->component.deinit();
}

void Stm32UartDriverTester ::resetStubState() {
    Stub_hwOpenSucceeds = true;
    Stub_hwOpenFailureStatus = 1;
    Stub_lastOpenedInstance = UsartInstance::Usart1;
    Stub_hwStartTxSucceeds = true;
    Stub_hwStartTxFailureStatus = 1;
    Stub_hwRestartRxStatus = 0;
    Stub_lastTxLen = 0;
    for (FwSizeType i = 0; i < sizeof(Stub_lastTxData); i++) {
        Stub_lastTxData[i] = 0;
    }
    Stub_hwAbortTxCallCount = 0;
    Stub_hwAbortRxCallCount = 0;
    Stub_hwInvalidateRxStagingCallCount = 0;
    Stub_hwClearUartErrorCallCount = 0;

    this->m_allocateReturnsValid = false;
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void Stm32UartDriverTester ::testOpenSuccess() {
    // Usart2, not the default Usart1: proves the requested instance really
    // propagates to the HAL boundary rather than the wiring accidentally
    // working only because Usart1 happens to be everyone's default.
    const Fw::Success status = this->component.open(64, UsartInstance::Usart2, 0, 0, 115200);
    ASSERT_EQ(status, Fw::Success::SUCCESS);
    ASSERT_EQ(Stub_lastOpenedInstance, UsartInstance::Usart2);
    ASSERT_EVENTS_PortOpened_SIZE(1);
    ASSERT_EVENTS_PortOpened(0, 115200);
    ASSERT_from_ready_SIZE(1);
}

void Stm32UartDriverTester ::testOpenFailure() {
    Stub_hwOpenSucceeds = false;
    Stub_hwOpenFailureStatus = 2;  // arbitrary HAL_StatusTypeDef stand-in

    const Fw::Success status = this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);
    ASSERT_EQ(status, Fw::Success::FAILURE);
    ASSERT_EVENTS_HalError_SIZE(1);
    ASSERT_EVENTS_HalError(0, "ReceiveToIdle_DMA", 2);
    ASSERT_EVENTS_PortOpened_SIZE(0);
    ASSERT_from_ready_SIZE(0);
}

void Stm32UartDriverTester ::testSendInvalidBuffer() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);

    Fw::Buffer invalid;  // default-constructed: no backing storage, not valid
    const Drv::ByteStreamStatus status = this->invoke_to_send(0, invalid);
    ASSERT_EQ(status, Drv::ByteStreamStatus::OTHER_ERROR);
    ASSERT_EVENTS_TxRingFull_SIZE(0);  // rejected before ever touching the ring
}

void Stm32UartDriverTester ::testSendFits() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);

    U8 data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    Fw::Buffer buffer(data, sizeof(data));
    const Drv::ByteStreamStatus status = this->invoke_to_send(0, buffer);
    ASSERT_EQ(status, Drv::ByteStreamStatus::OP_OK);
    ASSERT_EVENTS_TxRingFull_SIZE(0);
}

void Stm32UartDriverTester ::testSendRejectedWhenFull() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);

    // TX_RING_SIZE is 4096 (UartDriverConfig.hpp) -- two 4000-byte sends
    // cannot both fit.
    static U8 data[4000] = {0};
    Fw::Buffer first(data, sizeof(data));
    ASSERT_EQ(this->invoke_to_send(0, first), Drv::ByteStreamStatus::OP_OK);

    Fw::Buffer second(data, sizeof(data));
    const Drv::ByteStreamStatus status = this->invoke_to_send(0, second);
    ASSERT_EQ(status, Drv::ByteStreamStatus::SEND_RETRY);
    ASSERT_EVENTS_TxRingFull_SIZE(1);
}

void Stm32UartDriverTester ::testPollDrainsTx() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);

    U8 data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    Fw::Buffer buffer(data, sizeof(data));
    ASSERT_EQ(this->invoke_to_send(0, buffer), Drv::ByteStreamStatus::OP_OK);

    this->component.poll();  // hwStartTx() (stubbed) reports success

    // Common.cpp must have staged exactly these 8 bytes into the DMA
    // boundary -- not just "some" bytes, and not more than requested.
    ASSERT_EQ(Stub_lastTxLen, 8u);
    for (FwSizeType i = 0; i < 8; i++) {
        ASSERT_EQ(Stub_lastTxData[i], data[i]);
    }

    this->invoke_to_run(0, 0);
    ASSERT_TLM_BytesSent_SIZE(1);
    ASSERT_TLM_BytesSent(0, 8u);
    ASSERT_TLM_TxErrorCount(0, 0u);
}

void Stm32UartDriverTester ::testPollTxHwStartFailure() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);

    U8 data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    Fw::Buffer buffer(data, sizeof(data));
    ASSERT_EQ(this->invoke_to_send(0, buffer), Drv::ByteStreamStatus::OP_OK);

    Stub_hwStartTxSucceeds = false;
    Stub_hwStartTxFailureStatus = 3;
    this->component.poll();

    ASSERT_EVENTS_HalError_SIZE(1);
    ASSERT_EVENTS_HalError(0, "Transmit_DMA", 3);

    this->invoke_to_run(0, 0);
    ASSERT_TLM_TxErrorCount(0, 1u);
    ASSERT_TLM_BytesSent(0, 0u);  // never counted as sent since the HAL rejected it
}

void Stm32UartDriverTester ::testPollTxBusyThenComplete() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);

    U8 data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    Fw::Buffer buffer(data, sizeof(data));
    ASSERT_EQ(this->invoke_to_send(0, buffer), Drv::ByteStreamStatus::OP_OK);

    this->component.poll();  // kicks off the transfer; m_txDmaBusy becomes true
    ASSERT_EQ(Stub_lastTxLen, 8u);

    // A second poll() while still "busy" must not start a new transfer: it
    // should hit the busy/watchdog branch and return without touching the
    // HAL boundary again. Stub_lastTxLen is a sentinel here -- if pollTx()
    // wrongly re-entered the "start a transfer" path, it would change.
    Stub_lastTxLen = 0;
    this->component.poll();
    ASSERT_EQ(Stub_lastTxLen, 0u);

    // signalTxComplete() (the target of the real ISR trampoline) is what
    // frees the driver to send the next chunk.
    this->component.signalTxComplete();

    U8 more[3] = {9, 10, 11};
    Fw::Buffer secondBuffer(more, sizeof(more));
    ASSERT_EQ(this->invoke_to_send(0, secondBuffer), Drv::ByteStreamStatus::OP_OK);
    this->component.poll();
    ASSERT_EQ(Stub_lastTxLen, 3u);
}

void Stm32UartDriverTester ::testTxWatchdogTimeout() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);

    U8 data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    Fw::Buffer buffer(data, sizeof(data));
    ASSERT_EQ(this->invoke_to_send(0, buffer), Drv::ByteStreamStatus::OP_OK);

    this->component.poll();  // kicks off the transfer; m_txDmaBusy becomes true
    ASSERT_EQ(Stub_lastTxLen, 8u);

    // The watchdog for an 8-byte, 115200-baud transfer is a few
    // milliseconds (2x on-wire time + a fixed slack); wait comfortably
    // longer than that. The stub never calls signalTxComplete() on its
    // own, so the transfer is still "in flight" and the watchdog must fire.
    (void)Os::Task::delay(Fw::TimeInterval(0, 20000));  // 20 ms
    this->component.poll();

    ASSERT_EVENTS_TxTimeout_SIZE(1);

    this->invoke_to_run(0, 0);
    ASSERT_TLM_TxErrorCount(0, 1u);

    // The driver must have recovered, not gotten stuck: a fresh send now
    // succeeds and starts a new transfer.
    Stub_lastTxLen = 0;
    U8 more[2] = {9, 10};
    Fw::Buffer secondBuffer(more, sizeof(more));
    ASSERT_EQ(this->invoke_to_send(0, secondBuffer), Drv::ByteStreamStatus::OP_OK);
    this->component.poll();
    ASSERT_EQ(Stub_lastTxLen, 2u);
}

void Stm32UartDriverTester ::testRecvReturnIn() {
    U8 backing[4] = {0};
    Fw::Buffer buffer(backing, sizeof(backing));
    this->invoke_to_recvReturnIn(0, buffer);
    ASSERT_from_deallocate_SIZE(1);
    ASSERT_from_deallocate(0, buffer);
}

void Stm32UartDriverTester ::testPollDrainsRx() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);
    this->m_allocateReturnsValid = true;

    // No real DMA on the host to fill m_rxStaging -- stage the bytes
    // directly (Stm32UartDriverTester is a friend for exactly this).
    const FwSizeType len = 4;
    this->component.m_rxStaging[0] = 0xAA;
    this->component.m_rxStaging[1] = 0xBB;
    this->component.m_rxStaging[2] = 0xCC;
    this->component.m_rxStaging[3] = 0xDD;

    this->component.signalRxChunk(len);
    this->component.poll();

    ASSERT_EQ(Stub_hwInvalidateRxStagingCallCount, 1u);

    ASSERT_from_recv_SIZE(1);
    const auto& recvEntry = this->fromPortHistory_recv->at(0);
    ASSERT_EQ(recvEntry.buffer.getData(), this->m_allocateBacking);
    ASSERT_EQ(recvEntry.buffer.getSize(), len);
    ASSERT_EQ(recvEntry.status, Drv::ByteStreamStatus::OP_OK);
    ASSERT_EQ(this->m_allocateBacking[0], 0xAA);
    ASSERT_EQ(this->m_allocateBacking[1], 0xBB);
    ASSERT_EQ(this->m_allocateBacking[2], 0xCC);
    ASSERT_EQ(this->m_allocateBacking[3], 0xDD);

    this->invoke_to_run(0, 0);
    ASSERT_TLM_BytesRecv_SIZE(1);
    ASSERT_TLM_BytesRecv(0, len);
}

void Stm32UartDriverTester ::testPollRxNoBuffers() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);
    this->m_allocateReturnsValid = false;  // matches the GTestBase default, spelled out for clarity

    this->component.signalRxChunk(4);
    this->component.poll();

    ASSERT_from_recv_SIZE(0);
    ASSERT_EVENTS_NoBuffers_SIZE(1);
}

void Stm32UartDriverTester ::testPollRxRingFull() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);
    // m_allocateReturnsValid stays false: nothing drains, so the ring fills
    // up and stays full across every signalRxChunk()/poll() below.

    // Fill the 4096-byte RX ring exactly, in four 1024-byte staging-sized
    // chunks (RX_STAGING_SIZE, the real m_rxStaging's capacity -- signalling
    // a larger chunk than that would read past the staging buffer below).
    for (int i = 0; i < 4; i++) {
        this->component.signalRxChunk(Stm32UartDriverConfig::RX_STAGING_SIZE);
        this->component.poll();
    }
    ASSERT_EVENTS_RxRingFull_SIZE(0);

    // A fifth chunk has nowhere to go: freeSpace is 0, so all 10 bytes of
    // it are dropped and counted.
    this->component.signalRxChunk(10);
    this->component.poll();

    ASSERT_EVENTS_RxRingFull_SIZE(1);
    ASSERT_EVENTS_RxRingFull(0, 10);

    this->invoke_to_run(0, 0);
    ASSERT_TLM_RxErrorCount(0, 1u);
}

void Stm32UartDriverTester ::testUartErrorRecoveryRxOnly() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);

    this->component.signalUartError(0x08U);  // mirrors HAL_UART_ERROR_ORE, an RX-affecting error
    this->component.poll();

    ASSERT_EVENTS_UartError_SIZE(1);
    ASSERT_EVENTS_UartError(0, 0x08U);

    ASSERT_EQ(Stub_hwAbortTxCallCount, 1u);
    ASSERT_EQ(Stub_hwAbortRxCallCount, 1u);
    ASSERT_EQ(Stub_hwClearUartErrorCallCount, 1u);

    this->invoke_to_run(0, 0);
    ASSERT_TLM_RxErrorCount_SIZE(1);
    ASSERT_TLM_RxErrorCount(0, 1u);
    ASSERT_TLM_TxErrorCount(0, 0u);
}

void Stm32UartDriverTester ::testUartErrorRecoveryDmaError() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);

    this->component.signalUartError(0x10U);  // mirrors HAL_UART_ERROR_DMA
    this->component.poll();

    ASSERT_EVENTS_UartError_SIZE(1);
    ASSERT_EVENTS_UartError(0, 0x10U);

    ASSERT_EQ(Stub_hwAbortTxCallCount, 1u);
    ASSERT_EQ(Stub_hwAbortRxCallCount, 1u);
    ASSERT_EQ(Stub_hwClearUartErrorCallCount, 1u);

    this->invoke_to_run(0, 0);
    ASSERT_TLM_TxErrorCount(0, 1u);
    ASSERT_TLM_RxErrorCount(0, 1u);
}

void Stm32UartDriverTester ::testTwoInstancesDoNotInterfere() {
    (void)this->component.open(64, UsartInstance::Usart1, 0, 0, 115200);

    // A second, freestanding instance on a different USART. Before the
    // multi-instance fix, the real HAL boundary's ISR callback trampoline
    // cached a single {handle, component} pair shared by every
    // Stm32UartDriver in the process -- opening this second instance would
    // have silently rerouted the first instance's DMA-completion/RX/error
    // callbacks to itself.
    Stm32UartDriver secondComponent("Stm32UartDriverSecond");
    const Fw::Success secondOpenStatus = secondComponent.open(64, UsartInstance::Usart2, 0, 0, 115200);
    ASSERT_EQ(secondOpenStatus, Fw::Success::SUCCESS);

    // The first instance must still be fully functional, unaffected by the
    // second instance's later open().
    U8 data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    Fw::Buffer buffer(data, sizeof(data));
    const Drv::ByteStreamStatus status = this->invoke_to_send(0, buffer);
    ASSERT_EQ(status, Drv::ByteStreamStatus::OP_OK);
}

Fw::Buffer Stm32UartDriverTester ::from_allocate_handler(FwIndexType portNum, FwSizeType size) {
    this->pushFromPortEntry_allocate(size);
    if (!this->m_allocateReturnsValid) {
        return Fw::Buffer();
    }
    FW_ASSERT(size <= sizeof(this->m_allocateBacking), static_cast<FwAssertArgType>(size));
    return Fw::Buffer(this->m_allocateBacking, size);
}

}  // namespace Stm32
