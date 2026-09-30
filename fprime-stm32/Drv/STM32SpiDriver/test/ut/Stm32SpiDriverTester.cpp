// ======================================================================
// \title  Stm32SpiDriverTester.cpp
// \author ivanlara
// \brief  cpp file for Stm32SpiDriver component test harness implementation class
// ======================================================================

#include "Stm32SpiDriverTester.hpp"

// Stub_* globals defined in Stm32SpiDriverStub.cpp -- declared here so this
// Tester can inject HAL boundary success/failure and inspect exactly what
// Stm32SpiDriverCommon.cpp drove through it, instead of only exercising the
// stub's fixed defaults.
extern bool Stub_hwOpenSucceeds;
extern I32 Stub_hwTransmitReceiveStatus;
extern U8 Stub_rxResponseData[32];
extern Stm32::SpiInstance Stub_lastOpenedInstance;
extern Stm32::GpioPort Stub_lastCsPort;
extern U16 Stub_lastCsPin;
extern bool Stub_csActiveDuringTransfer;
extern U8 Stub_lastTxData[32];
extern FwSizeType Stub_lastTxLen;

namespace Stm32 {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

Stm32SpiDriverTester ::Stm32SpiDriverTester()
    : Stm32SpiDriverGTestBase("Stm32SpiDriverTester", Stm32SpiDriverTester::MAX_HISTORY_SIZE),
      component("Stm32SpiDriver") {
    this->resetStubState();
    this->initComponents();
    this->connectPorts();
}

Stm32SpiDriverTester ::~Stm32SpiDriverTester() {
    this->component.deinit();
}

void Stm32SpiDriverTester ::resetStubState() {
    Stub_hwOpenSucceeds = true;
    Stub_hwTransmitReceiveStatus = 0;
    for (FwSizeType i = 0; i < sizeof(Stub_rxResponseData); i++) {
        Stub_rxResponseData[i] = 0;
    }
    Stub_lastOpenedInstance = SpiInstance::Spi5;
    Stub_lastCsPort = Stm32::GpioPort::A;
    Stub_lastCsPin = 0;
    Stub_csActiveDuringTransfer = false;
    for (FwSizeType i = 0; i < sizeof(Stub_lastTxData); i++) {
        Stub_lastTxData[i] = 0;
    }
    Stub_lastTxLen = 0;
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void Stm32SpiDriverTester ::testOpenSuccess() {
    const Fw::Success status = this->component.open(SpiInstance::Spi5, Stm32::GpioPort::F, 6);
    ASSERT_EQ(status, Fw::Success::SUCCESS);
    ASSERT_EQ(Stub_lastOpenedInstance, SpiInstance::Spi5);
    ASSERT_EQ(Stub_lastCsPort, Stm32::GpioPort::F);
    ASSERT_EQ(Stub_lastCsPin, 6u);
    ASSERT_EVENTS_PortOpened_SIZE(1);
    ASSERT_EVENTS_PortOpened(0, "Spi5");
}

void Stm32SpiDriverTester ::testOpenFailure() {
    Stub_hwOpenSucceeds = false;
    const Fw::Success status = this->component.open(SpiInstance::Spi5, Stm32::GpioPort::F, 6);
    ASSERT_EQ(status, Fw::Success::FAILURE);
    ASSERT_EVENTS_PortOpened_SIZE(0);
}

void Stm32SpiDriverTester ::testSpiWriteReadBeforeOpen() {
    U8 wbacking[4] = {1, 2, 3, 4};
    U8 rbacking[4] = {0};
    Fw::Buffer writeBuffer(wbacking, sizeof(wbacking));
    Fw::Buffer readBuffer(rbacking, sizeof(rbacking));
    ASSERT_EQ(this->invoke_to_SpiWriteRead(0, writeBuffer, readBuffer), Drv::SpiStatus::SPI_OPEN_ERR);
}

void Stm32SpiDriverTester ::testSpiReadWriteBeforeOpen() {
    U8 wbacking[4] = {1, 2, 3, 4};
    U8 rbacking[4] = {0};
    Fw::Buffer writeBuffer(wbacking, sizeof(wbacking));
    Fw::Buffer readBuffer(rbacking, sizeof(rbacking));
    // SpiReadWrite has no return value: absence of a crash and an
    // unaffected read buffer confirm the OPEN_ERR short-circuit ran.
    this->invoke_to_SpiReadWrite(0, writeBuffer, readBuffer);
    ASSERT_EQ(rbacking[0], 0u);
}

void Stm32SpiDriverTester ::testSpiWriteReadSuccess() {
    (void)this->component.open(SpiInstance::Spi5, Stm32::GpioPort::F, 6);
    Stub_rxResponseData[0] = 0x58;  // BMP280 chip-id-register style response
    Stub_rxResponseData[1] = 0x11;

    U8 wdata[2] = {0xD0, 0x00};
    U8 rbacking[2] = {0};
    Fw::Buffer writeBuffer(wdata, sizeof(wdata));
    Fw::Buffer readBuffer(rbacking, sizeof(rbacking));
    const Drv::SpiStatus status = this->invoke_to_SpiWriteRead(0, writeBuffer, readBuffer);
    ASSERT_EQ(status, Drv::SpiStatus::SPI_OK);

    // Chip-select must have been asserted for the exact duration of the
    // transfer -- a driver that forgets to call hwSetCs(true) before
    // transferring would leave this false.
    ASSERT_TRUE(Stub_csActiveDuringTransfer);
    ASSERT_EQ(Stub_lastTxLen, 2u);
    ASSERT_EQ(Stub_lastTxData[0], 0xD0);
    ASSERT_EQ(Stub_lastTxData[1], 0x00);
    ASSERT_EQ(rbacking[0], 0x58);
    ASSERT_EQ(rbacking[1], 0x11);
}

void Stm32SpiDriverTester ::testSpiWriteReadFailure() {
    (void)this->component.open(SpiInstance::Spi5, Stm32::GpioPort::F, 6);
    Stub_hwTransmitReceiveStatus = 1;  // any non-zero (non-HAL_OK) status

    U8 wdata[1] = {0xD0};
    U8 rbacking[1] = {0};
    Fw::Buffer writeBuffer(wdata, sizeof(wdata));
    Fw::Buffer readBuffer(rbacking, sizeof(rbacking));
    const Drv::SpiStatus status = this->invoke_to_SpiWriteRead(0, writeBuffer, readBuffer);
    ASSERT_EQ(status, Drv::SpiStatus::SPI_WRITE_ERR);
    ASSERT_EVENTS_HalError_SIZE(1);
    ASSERT_EVENTS_HalError(0, "TransmitReceive", 1);
}

void Stm32SpiDriverTester ::testSpiReadWriteSuccess() {
    (void)this->component.open(SpiInstance::Spi5, Stm32::GpioPort::F, 6);
    Stub_rxResponseData[0] = 0x42;

    U8 wdata[1] = {0xD0};
    U8 rbacking[1] = {0};
    Fw::Buffer writeBuffer(wdata, sizeof(wdata));
    Fw::Buffer readBuffer(rbacking, sizeof(rbacking));
    this->invoke_to_SpiReadWrite(0, writeBuffer, readBuffer);

    ASSERT_TRUE(Stub_csActiveDuringTransfer);
    ASSERT_EQ(rbacking[0], 0x42);
}

void Stm32SpiDriverTester ::testSpiWriteReadLargeBufferCapped() {
    (void)this->component.open(SpiInstance::Spi5, Stm32::GpioPort::F, 6);

    // Larger than Stub_lastTxData's 32-byte capture buffer.
    U8 wdata[40];
    U8 rbacking[40] = {0};
    for (FwSizeType i = 0; i < sizeof(wdata); i++) {
        wdata[i] = static_cast<U8>(i);
    }
    Fw::Buffer writeBuffer(wdata, sizeof(wdata));
    Fw::Buffer readBuffer(rbacking, sizeof(rbacking));
    const Drv::SpiStatus status = this->invoke_to_SpiWriteRead(0, writeBuffer, readBuffer);
    ASSERT_EQ(status, Drv::SpiStatus::SPI_OK);

    // The stub must truncate its capture rather than overrun its own
    // 32-byte buffer; the real HAL call underneath would see the full 40.
    ASSERT_EQ(Stub_lastTxLen, 40u);
    for (FwSizeType i = 0; i < 32; i++) {
        ASSERT_EQ(Stub_lastTxData[i], wdata[i]);
    }
}

}  // namespace Stm32
