// ======================================================================
// \title  Stm32UartDriverTester.hpp
// \author ivanlara
// \brief  hpp file for Stm32UartDriver component test harness implementation class
// ======================================================================

#ifndef Stm32_Stm32UartDriverTester_HPP
#define Stm32_Stm32UartDriverTester_HPP

#include "fprime-stm32/Drv/STM32UartDriver/Stm32UartDriver.hpp"
#include "fprime-stm32/Drv/STM32UartDriver/Stm32UartDriverGTestBase.hpp"

namespace Stm32 {

class Stm32UartDriverTester final : public Stm32UartDriverGTestBase {
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

    //! Construct object Stm32UartDriverTester
    Stm32UartDriverTester();

    //! Destroy object Stm32UartDriverTester
    ~Stm32UartDriverTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! open() succeeds, reports the actual baud, emits PortOpened/ready, and
    //! forwards the requested instance to the HAL boundary unmodified
    void testOpenSuccess();

    //! open() reports FAILURE and emits HalError when the HAL boundary's
    //! initial RX arm fails (hwOpen failure injected via Stub_hwOpenSucceeds)
    void testOpenFailure();

    //! send() rejects an invalid (unset) Fw::Buffer without touching the ring
    void testSendInvalidBuffer();

    //! send() enqueues into the TX ring and returns OP_OK
    void testSendFits();

    //! send() rejects a request that would overflow the TX ring
    void testSendRejectedWhenFull();

    //! poll() drains the TX ring into a (stubbed) DMA transfer -- reflected
    //! in BytesSent telemetry on the next run() tick, and in the exact bytes
    //! captured by the HAL boundary (Stub_lastTxData/Stub_lastTxLen)
    void testPollDrainsTx();

    //! poll() reports HalError and counts a TX error when the HAL boundary
    //! rejects the transfer (hwStartTx failure injected via Stub_hwStartTxSucceeds)
    void testPollTxHwStartFailure();

    //! A second poll() while a transfer is still in flight must not start
    //! another one; signalTxComplete() (the ISR trampoline's target) is
    //! what frees the driver to send the next chunk
    void testPollTxBusyThenComplete();

    //! A transfer that never completes (signalTxComplete() never called)
    //! is aborted once real elapsed time exceeds its watchdog
    void testTxWatchdogTimeout();

    //! recvReturnIn() returns the caller's buffer ownership via deallocate()
    void testRecvReturnIn();

    //! signalRxChunk() + poll() drains staged bytes to recv() with a
    //! successfully allocated buffer, invalidating the D-cache exactly once
    void testPollDrainsRx();

    //! poll() reports NoBuffers and leaves data queued when allocate()
    //! cannot supply a buffer
    void testPollRxNoBuffers();

    //! A chunk that arrives with no free space left in the RX ring is
    //! (partially) dropped and reported via RxRingFull/RxErrorCount
    void testPollRxRingFull();

    //! signalUartError() + poll() recovers an RX-affecting (non-DMA) error:
    //! counts only RxErrorCount and runs the abort/clear recovery sequence
    //! exactly once
    void testUartErrorRecoveryRxOnly();

    //! signalUartError() + poll() recovers a DMA error: counts both
    //! TxErrorCount and RxErrorCount
    void testUartErrorRecoveryDmaError();

    //! Two simultaneously-open Stm32UartDriver instances (different USART
    //! peripherals, e.g. USART1 for the ground link and USART2 for a second
    //! radio) don't share any mutable state: opening a second instance
    //! after the first is already open must not disturb the first
    //! instance's own ability to send.
    void testTwoInstancesDoNotInterfere();

  private:
    // ----------------------------------------------------------------------
    // Test support
    // ----------------------------------------------------------------------

    //! Override: by default the base class returns an invalid Fw::Buffer;
    //! tests that need allocate() to succeed set m_allocateReturnsValid.
    Fw::Buffer from_allocate_handler(FwIndexType portNum, FwSizeType size) override;

    //! Reset every Stub_* global back to its documented default so each
    //! test starts from a known, hermetic state regardless of run order --
    //! these are shared, process-wide globals (see Stm32UartDriverStub.cpp),
    //! not per-Tester state.
    void resetStubState();

    bool m_allocateReturnsValid = false;
    U8 m_allocateBacking[256] = {0};

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
    Stm32UartDriver component;
};

}  // namespace Stm32

#endif
