// ======================================================================
// \title  Stm32UartDriverCommon.cpp
// \author ivanlara
// \brief  Hardware-independent logic for the STM32H7 USART1 DMA-backed
//         byte stream driver: shared by the real (stm32h7) and stubbed
//         (host UT) builds. Contains no HAL/CMSIS dependency -- every
//         register/DMA touch is behind the hw*() boundary declared in
//         Stm32UartDriver.hpp and implemented in Stm32UartDriver.cpp
//         (real) / Stm32UartDriverStub.cpp (host).
// ======================================================================

#include <fprime-stm32/Drv/STM32UartDriver/Stm32UartDriver.hpp>

namespace Stm32 {

// ----------------------------------------------------------------------
// Construction, initialization, and destruction
// ----------------------------------------------------------------------

Stm32UartDriver ::Stm32UartDriver(const char* const compName)
    : Stm32UartDriverComponentBase(compName),
      m_instance(UsartInstance::Usart1),
      m_txHead(0),
      m_txCount(0),
      m_txTimeoutUs(0),
      m_baudRate(0),
      m_txDmaBusy(false),
      m_rxChunkReady(false),
      m_rxChunkLen(0),
      m_uartErrorPending(false),
      m_uartErrorCode(0),
      m_rxHead(0),
      m_rxCount(0),
      m_allocationSize(0),
      m_bytesSent(0),
      m_bytesReceived(0),
      m_txErrorCount(0),
      m_rxErrorCount(0) {}

Stm32UartDriver ::~Stm32UartDriver() {}

void Stm32UartDriver ::signalTxComplete() {
    this->m_txDmaBusy = false;
}

void Stm32UartDriver ::signalRxChunk(FwSizeType len) {
    this->m_rxChunkLen = len;
    this->m_rxChunkReady = true;
}

void Stm32UartDriver ::signalUartError(U32 errorCode) {
    this->m_uartErrorCode = errorCode;
    this->m_uartErrorPending = true;
}

Fw::Success Stm32UartDriver ::open(FwSizeType allocationSize, UsartInstance instance, U32 preemptPriority,
                                    U32 subPriority, U32 baudRate) {
    this->m_instance = instance;
    this->m_allocationSize = allocationSize;

    this->m_baudRate = baudRate;
    FW_ASSERT(this->m_baudRate > 0);

    U32 actualBaudRate = 0;
    if (!this->hwOpen(instance, preemptPriority, subPriority, baudRate, actualBaudRate)) {
        return Fw::Success::FAILURE;
    }

    this->log_ACTIVITY_HI_PortOpened(actualBaudRate);
    if (this->isConnected_ready_OutputPort(0)) {
        this->ready_out(0);
    }
    return Fw::Success::SUCCESS;
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined typed input ports
// ----------------------------------------------------------------------

void Stm32UartDriver ::poll() {
    if (this->m_uartErrorPending) {
        this->recoverUartError();
    }

    this->pollRx();
    this->pollTx();
}

void Stm32UartDriver ::run_handler(FwIndexType portNum, U32 context) {
    this->tlmWrite_BytesSent(this->m_bytesSent);
    this->tlmWrite_BytesRecv(this->m_bytesReceived);
    this->tlmWrite_TxErrorCount(this->m_txErrorCount);
    this->tlmWrite_RxErrorCount(this->m_rxErrorCount);
}

Drv::ByteStreamStatus Stm32UartDriver ::send_handler(FwIndexType portNum, Fw::Buffer& serBuffer) {
    if (!serBuffer.isValid()) {
        return Drv::ByteStreamStatus::OTHER_ERROR;
    }

    const FwSizeType size = serBuffer.getSize();
    const FwSizeType free = TX_RING_SIZE - this->m_txCount;
    if (size > free) {
        this->log_WARNING_HI_TxRingFull(static_cast<U32>(size), static_cast<U32>(free));
        return Drv::ByteStreamStatus::SEND_RETRY;
    }

    const U8* data = serBuffer.getData();
    for (FwSizeType i = 0; i < size; i++) {
        this->m_txRing[(this->m_txHead + i) % TX_RING_SIZE] = data[i];
    }
    this->m_txHead = (this->m_txHead + size) % TX_RING_SIZE;
    this->m_txCount += size;

    return Drv::ByteStreamStatus::OP_OK;
}

void Stm32UartDriver ::recvReturnIn_handler(FwIndexType portNum, Fw::Buffer& fwBuffer) {
    this->deallocate_out(0, fwBuffer);
}

// ----------------------------------------------------------------------
// TX path
// ----------------------------------------------------------------------

void Stm32UartDriver ::pollTx() {
    if (this->m_txDmaBusy) {
        Os::RawTime now;
        (void)now.now();
        U32 elapsedUs = 0;
        (void)now.getDiffUsec(this->m_txDmaStart, elapsedUs);
        if (elapsedUs >= this->m_txTimeoutUs) {
            this->hwAbortTx();
            this->m_txDmaBusy = false;
            this->m_txErrorCount++;
            this->log_WARNING_HI_TxTimeout(elapsedUs);
        }
        return;
    }

    if (this->m_txCount == 0) {
        return;
    }

    const FwSizeType toSend = (this->m_txCount < TX_STAGING_SIZE) ? this->m_txCount : TX_STAGING_SIZE;
    const FwSizeType tail = (this->m_txHead + TX_RING_SIZE - this->m_txCount) % TX_RING_SIZE;
    for (FwSizeType i = 0; i < toSend; i++) {
        this->m_txStaging[i] = this->m_txRing[(tail + i) % TX_RING_SIZE];
    }
    this->m_txCount -= toSend;

    const U32 onWireUs =
        static_cast<U32>((static_cast<U64>(toSend) * TX_BITS_PER_BYTE * 1000000U) / this->m_baudRate);
    this->m_txTimeoutUs = (onWireUs * TX_TIMEOUT_MARGIN) + TX_TIMEOUT_SLACK_US;

    (void)this->m_txDmaStart.now();
    this->m_txDmaBusy = true;
    if (this->hwStartTx(this->m_txStaging, toSend)) {
        this->m_bytesSent += toSend;
    } else {
        this->m_txDmaBusy = false;
        this->m_txErrorCount++;
    }
}

// ----------------------------------------------------------------------
// RX path
// ----------------------------------------------------------------------

void Stm32UartDriver ::pollRx() {
    if (this->m_rxChunkReady) {
        const FwSizeType len = this->m_rxChunkLen;
        this->m_rxChunkReady = false;
        this->m_rxChunkLen = 0;

        this->hwInvalidateRxStaging();

        const FwSizeType freeSpace = RX_RING_SIZE - this->m_rxCount;
        const FwSizeType toCopy = (len < freeSpace) ? len : freeSpace;
        for (FwSizeType i = 0; i < toCopy; i++) {
            this->m_rxRing[(this->m_rxHead + i) % RX_RING_SIZE] = this->m_rxStaging[i];
        }
        this->m_rxHead = (this->m_rxHead + toCopy) % RX_RING_SIZE;
        this->m_rxCount += toCopy;

        if (toCopy < len) {
            this->m_rxErrorCount++;
            this->log_WARNING_HI_RxRingFull(static_cast<U32>(len - toCopy));
        }

        // Re-arm immediately so no bytes are lost while the ring/Fw::Buffer
        // drain below runs.
        (void)this->hwRestartRx();
    }

    if (this->m_rxCount == 0) {
        return;
    }

    Fw::Buffer buff = this->allocate_out(0, this->m_allocationSize);
    if (buff.getData() == nullptr) {
        this->log_WARNING_HI_NoBuffers();
        return;  // leave data queued in the ring; retry next poll
    }

    const FwSizeType tail = (this->m_rxHead + RX_RING_SIZE - this->m_rxCount) % RX_RING_SIZE;
    const FwSizeType copySize = (this->m_rxCount < buff.getSize()) ? this->m_rxCount : buff.getSize();
    for (FwSizeType i = 0; i < copySize; i++) {
        buff.getData()[i] = this->m_rxRing[(tail + i) % RX_RING_SIZE];
    }
    this->m_rxCount -= copySize;

    buff.setSize(copySize);
    this->m_bytesReceived += copySize;
    this->recv_out(0, buff, Drv::ByteStreamStatus::OP_OK);
}

void Stm32UartDriver ::recoverUartError() {
    const U32 errorCode = this->m_uartErrorCode;
    this->m_uartErrorPending = false;
    this->m_uartErrorCode = 0;

    this->hwAbortTx();
    this->hwAbortRx();
    this->m_txDmaBusy = false;

    bool isRxAffecting = false;
    bool isDmaAffecting = false;
    this->hwClassifyUartError(errorCode, isRxAffecting, isDmaAffecting);
    if (isRxAffecting) {
        this->m_rxErrorCount++;
    }
    if (isDmaAffecting) {
        this->m_txErrorCount++;
        this->m_rxErrorCount++;
    }
    this->log_WARNING_HI_UartError(errorCode);

    this->hwClearUartError();

    this->m_rxChunkReady = false;
    this->m_rxChunkLen = 0;
    (void)this->hwRestartRx();
}

}  // namespace Stm32
