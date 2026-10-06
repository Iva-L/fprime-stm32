// ======================================================================
// \title  STM32UartDriver.hpp
// \author ivanlara
// \brief  hpp file for the STM32H7 USART1 DMA-backed byte stream driver
// ======================================================================

#ifndef STM32_UART_DRIVER_HPP
#define STM32_UART_DRIVER_HPP

#include <fprime-stm32/Drv/STM32UartDriver/Stm32UartDriverComponentAc.hpp>
#include <Fw/Types/SuccessEnumAc.hpp>
#include <Os/RawTime.hpp>
#include <config/UartDriverConfig.hpp>
#include <config/Stm32TransferMode.hpp>

namespace Stm32 {

//! USART/UART peripheral identifier, covering every instance present on an STM32H753
enum class UsartInstance { Usart1, Usart2, Usart3, Uart4, Uart5, Usart6, Uart7, Uart8 };

class Stm32UartDriver final : public Stm32UartDriverComponentBase {
  public:
    //! Construct object Stm32UartDriver
    explicit Stm32UartDriver(const char* const compName);

    //! Configure the selected USART/UART instance (via the CubeMX-generated
    //! MX_DMA_Init()/MX_USARTn_UART_Init(), fixed at the generated baud/frame
    //! settings in lib/fprime-stm32/src/usart.c) and arm the first RX
    //! reception. Must be called once from configureTopology().
    //! \param allocationSize size of the AXI SRAM ring buffers
    //! \param instance which USART/UART peripheral this driver instance owns;
    //!        the NVIC interrupt number is derived from this internally, so
    //!        the caller cannot pass a mismatched instance/IRQn pair
    //! \param preemptPriority NVIC preempt priority for the instance's global interrupt
    //! \param subPriority NVIC subpriority for the instance's global interrupt
    //! \param baudRate desired baud rate for the peripheral
    //! \param mode DMA (default, backward-compatible -- this driver has no
    //!        other behavior today) keeps the existing ring-buffered,
    //!        non-blocking idle-line DMA design, where `send` enqueues and
    //!        returns immediately and `poll()` drains/arms DMA in the
    //!        background. POLLED bypasses the ring/DMA entirely: `send`
    //!        calls a single blocking HAL_UART_Transmit() and returns only
    //!        once it completes (or times out), and `poll()` drains
    //!        whatever bytes are already in the peripheral via a
    //!        zero-timeout HAL_UART_Receive() peek each pass instead of
    //!        idle-line DMA -- an explicit, opt-in trade-off for low-rate/
    //!        debug UARTs where blocking `send` is acceptable.
    Fw::Success open(FwSizeType allocationSize, UsartInstance instance, U32 preemptPriority, U32 subPriority,
                      U32 baudRate, TransferMode mode = TransferMode::DMA);
    
    //! One bounded step of the DMA state machine: consume ISR-latched
    //! completion/error state, run cache maintenance, start the next transfer,
    //! and hand received data upstream. Called every cyclic-executive pass (not
    //! from a rate group) so DMA completions are serviced with minimal latency.
    void poll();

    //! Destroy object Stm32UartDriver
    ~Stm32UartDriver();

    // ----------------------------------------------------------------------
    // ISR signal surface: called by the real HAL callback trampoline (free
    // functions with no user-context pointer) on the stm32h7 target. A unit
    // test may also call these directly to simulate a hardware event, since
    // no ISR exists on the host.
    // ----------------------------------------------------------------------

    //! Signal that the in-flight TX DMA transfer completed.
    void signalTxComplete();

    //! Signal that an RX idle-line chunk of `len` bytes is ready to drain.
    void signalRxChunk(FwSizeType len);

    //! Signal that USART1/DMA latched the given HAL error code.
    void signalUartError(U32 errorCode);

  private:
    // ----------------------------------------------------------------------
    // HAL boundary: the only methods allowed to touch UART_HandleTypeDef/
    // DMA_HandleTypeDef/HAL calls or cache maintenance. Implemented once
    // against the real HAL in Stm32UartDriver.cpp (stm32h7 target only) and
    // once as a fixed-behavior stand-in in Stm32UartDriverStub.cpp (host
    // unit tests).
    // ----------------------------------------------------------------------

    //! Run MX_USARTn_UART_Init() for the selected instance. When `mode` is
    //! DMA: also run MX_DMA_Init(), configure that instance's global NVIC
    //! interrupt (IRQn derived from `instance`), register this instance for
    //! ISR callback routing, and arm the first RX reception -- none of
    //! which a POLLED instance needs. Returns true on success (matches
    //! today's HAL_OK checks) and reports the peripheral's actual
    //! configured baud via outActualBaudRate; emits HalError itself on
    //! failure since only this method knows which HAL call failed.
    bool hwOpen(UsartInstance instance, U32 preemptPriority, U32 subPriority, U32 requestedBaudRate, TransferMode mode,
                U32& outActualBaudRate);

    //! Clean the D-cache over [data, data + len) and start a TX DMA
    //! transfer out of it. Returns true if the HAL accepted the transfer.
    bool hwStartTx(const U8* data, FwSizeType len);

    //! Abort an in-flight TX DMA transfer (watchdog recovery).
    void hwAbortTx();

    //! Invalidate the D-cache over the RX staging buffer so the CPU reads
    //! what the DMA controller actually wrote.
    void hwInvalidateRxStaging();

    //! Re-arm idle-line RX reception into the RX staging buffer. Returns
    //! the raw HAL status (0 / HAL_OK on success) so hwOpen() can report an
    //! exact failure code; other call sites treat any nonzero as failure.
    I32 hwRestartRx();

    //! Abort an in-flight RX DMA transfer (error recovery).
    void hwAbortRx();

    //! Clear the latched USART1 HAL error code after it has been reported.
    void hwClearUartError();

    //! Classify a latched HAL error code into RX-affecting / DMA-affecting
    //! flags, matching the real HAL_UART_ERROR_* bitmask semantics without
    //! exposing the bitmask itself outside the HAL boundary.
    void hwClassifyUartError(U32 errorCode, bool& isRxAffecting, bool& isDmaAffecting);

    //! POLLED mode only: a single blocking HAL_UART_Transmit() of `len`
    //! bytes, bounded by `timeoutMs`. Returns true iff it completed
    //! successfully; no ring buffer, no DMA, no cache maintenance.
    bool hwPolledTransmit(const U8* data, FwSizeType len, U32 timeoutMs);

    //! POLLED mode only: a zero-timeout HAL_UART_Receive() of a single
    //! byte -- returns true and sets `outByte` iff a byte was already
    //! available in the peripheral, false immediately otherwise. Never
    //! blocks, so repeated calls from poll() are safe.
    bool hwPolledReceiveByte(U8& outByte);

    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Static configuration constants derived from UartDriverConfig.hpp.
    static constexpr FwSizeType TX_RING_SIZE = Stm32UartDriverConfig::TX_RING_SIZE;
    static constexpr FwSizeType TX_STAGING_SIZE = Stm32UartDriverConfig::TX_STAGING_SIZE;
    static constexpr U32 TX_BITS_PER_BYTE = Stm32UartDriverConfig::TX_BITS_PER_BYTE;
    static constexpr U32 TX_TIMEOUT_MARGIN = Stm32UartDriverConfig::TX_TIMEOUT_MARGIN;
    static constexpr U32 TX_TIMEOUT_SLACK_US = Stm32UartDriverConfig::TX_TIMEOUT_SLACK_US;
    static constexpr FwSizeType RX_RING_SIZE = Stm32UartDriverConfig::RX_RING_SIZE;
    static constexpr FwSizeType RX_STAGING_SIZE = Stm32UartDriverConfig::RX_STAGING_SIZE;

    //! Which USART/UART peripheral this instance owns, re-resolved to a real
    //! HAL handle on every hw* call rather than cached in file-static state
    //! -- this is what lets multiple Stm32UartDriver instances (e.g. USART1
    //! for the ground link and USART2 for a second radio) be open at the
    //! same time. The ISR callback trampoline (Stm32UartDriver.cpp) uses a
    //! small fixed-size registry indexed by this same enum, since the HAL
    //! callbacks themselves receive no instance/context, only a raw handle.
    UsartInstance m_instance;
    TransferMode m_transferMode;

    //! RX ring buffer and associated state.
    U8 m_rxRing[RX_RING_SIZE];
    FwSizeType m_rxHead;
    FwSizeType m_rxCount;

    //! TX ring buffer and associated state.
    U8 m_txRing[TX_RING_SIZE];
    FwSizeType m_txHead;   //!< next free slot to write
    FwSizeType m_txCount;  //!< bytes currently buffered

    //! TX DMA state and watchdog.
    Os::RawTime m_txDmaStart;
    U32 m_txTimeoutUs;  //!< watchdog for the in-flight transfer, sized from its length
    U32 m_baudRate;     //!< configured USART1 baud, cached for watchdog sizing

    //! Completion/error state latched by signalTxComplete()/signalRxChunk()/
    //! signalUartError() and consumed by poll()'s state machine below.
    volatile bool m_txDmaBusy;
    volatile bool m_rxChunkReady;
    FwSizeType m_rxChunkLen;
    volatile bool m_uartErrorPending;
    U32 m_uartErrorCode;

    //! Allocation and telemetry state.
    FwSizeType m_allocationSize;  //!< size of each Fw::Buffer allocation request
    
    //! Number of bytes sent and received, and error counts.
    FwSizeType m_bytesSent;
    FwSizeType m_bytesReceived;
    U32 m_txErrorCount;
    U32 m_rxErrorCount;


    //! Telemetry emission only, connected to a rate group. The DMA state
    //! machine runs in poll(), which needs a far higher cadence than any rate
    //! group provides.
    void run_handler(FwIndexType portNum, U32 context) override;

    //! Synchronous send: enqueues into the TX ring buffer and returns.
    //! Rejects the whole request (no partial enqueue) if it does not fit,
    //! so the caller's buffer is either fully consumed or fully retained.
    Drv::ByteStreamStatus send_handler(FwIndexType portNum, Fw::Buffer& serBuffer) override;

    //! Port receiving back ownership of data sent out on $recv port
    void recvReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) override;
    
    // ----------------------------------------------------------------------
    // TX path: ring buffer -> DMA staging buffer -> USART1 DMA
    // ----------------------------------------------------------------------

    //! Start the next TX DMA transfer if the ring has data and DMA is idle;
    //! recover a stuck transfer once it exceeds its watchdog.
    void pollTx();
    alignas(32) U8 m_txStaging[TX_STAGING_SIZE];

    // ----------------------------------------------------------------------
    // RX path: USART1 DMA -> DMA staging buffer -> ring buffer -> Fw::Buffer
    // ----------------------------------------------------------------------

    //! Drain a completed RX DMA staging chunk into the RX ring and re-arm
    //! reception, then forward whatever is queued in the ring upstream via
    //! allocate_out()/recv_out().
    void pollRx();

    //! Recover from a latched USART1/DMA error: abort in-flight transfers,
    //! clear HAL error state, report it, and re-arm reception.
    void recoverUartError();
    alignas(32) U8 m_rxStaging[RX_STAGING_SIZE];

    //! Unit-test access to m_rxStaging, to stage deterministic bytes ahead
    //! of a signalRxChunk() call (there is no real DMA on the host to fill
    //! it) -- see CPP-16 (friend only for unit-test access).
    friend class Stm32UartDriverTester;
};

}  // namespace Stm32

#endif
