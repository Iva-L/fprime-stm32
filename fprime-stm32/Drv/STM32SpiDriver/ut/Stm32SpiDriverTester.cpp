// ======================================================================
// \title  Stm32I2cDriverTester.cpp
// \author ivanlara
// \brief  cpp file for Stm32I2cDriver component test harness implementation class
// ======================================================================

#include "Stm32I2cDriverTester.hpp"

// Stub_* globals defined in Stm32I2cDriverStub.cpp -- declared here so this
// Tester can inject HAL boundary success/failure and inspect exactly what
// Stm32I2cDriverCommon.cpp drove through it, instead of only exercising the
// stub's fixed defaults.
extern bool Stub_hwOpenSucceeds;
extern Drv::I2cStatus Stub_hwMasterTransmitStatus;
extern Drv::I2cStatus Stub_hwMasterReceiveStatus;
extern U8 Stub_readResponseData[32];
extern Stm32::I2cInstance Stub_lastOpenedInstance;
extern Stm32::I2cBusSpeed Stub_lastRequestedBusSpeed;
extern U16 Stub_lastDevAddress;
extern U8 Stub_lastWriteData[32];
extern U16 Stub_lastWriteLen;
extern U16 Stub_lastReadLen;

namespace Stm32 {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

Stm32I2cDriverTester ::Stm32I2cDriverTester()
    : Stm32I2cDriverGTestBase("Stm32I2cDriverTester", Stm32I2cDriverTester::MAX_HISTORY_SIZE),
      component("Stm32I2cDriver") {
    this->resetStubState();
    this->initComponents();
    this->connectPorts();
}

Stm32I2cDriverTester ::~Stm32I2cDriverTester() {
    this->component.deinit();
}

void Stm32I2cDriverTester ::resetStubState() {
    Stub_hwOpenSucceeds = true;
    Stub_hwMasterTransmitStatus = Drv::I2cStatus::I2C_OK;
    Stub_hwMasterReceiveStatus = Drv::I2cStatus::I2C_OK;
    for (FwSizeType i = 0; i < sizeof(Stub_readResponseData); i++) {
        Stub_readResponseData[i] = 0;
    }
    Stub_lastOpenedInstance = I2cInstance::I2c1;
    Stub_lastRequestedBusSpeed = I2cBusSpeed::Fast;
    Stub_lastDevAddress = 0;
    for (FwSizeType i = 0; i < sizeof(Stub_lastWriteData); i++) {
        Stub_lastWriteData[i] = 0;
    }
    Stub_lastWriteLen = 0;
    Stub_lastReadLen = 0;
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void Stm32I2cDriverTester ::testOpenSuccessFast() {
    const Fw::Success status = this->component.open(I2cInstance::I2c1, I2cBusSpeed::Fast);
    ASSERT_EQ(status, Fw::Success::SUCCESS);
    ASSERT_EQ(Stub_lastOpenedInstance, I2cInstance::I2c1);
    ASSERT_EQ(Stub_lastRequestedBusSpeed, I2cBusSpeed::Fast);
    ASSERT_EVENTS_PortOpened_SIZE(1);
    ASSERT_EVENTS_PortOpened(0, "Fast");
}

void Stm32I2cDriverTester ::testOpenSuccessStandard() {
    const Fw::Success status = this->component.open(I2cInstance::I2c1, I2cBusSpeed::Standard);
    ASSERT_EQ(status, Fw::Success::SUCCESS);
    ASSERT_EQ(Stub_lastRequestedBusSpeed, I2cBusSpeed::Standard);
    ASSERT_EVENTS_PortOpened_SIZE(1);
    ASSERT_EVENTS_PortOpened(0, "Standard");
}

void Stm32I2cDriverTester ::testOpenSuccessFastPlus() {
    const Fw::Success status = this->component.open(I2cInstance::I2c1, I2cBusSpeed::FastPlus);
    ASSERT_EQ(status, Fw::Success::SUCCESS);
    ASSERT_EQ(Stub_lastRequestedBusSpeed, I2cBusSpeed::FastPlus);
    ASSERT_EVENTS_PortOpened_SIZE(1);
    ASSERT_EVENTS_PortOpened(0, "FastPlus");
}

void Stm32I2cDriverTester ::testOpenFailure() {
    Stub_hwOpenSucceeds = false;

    const Fw::Success status = this->component.open(I2cInstance::I2c1);
    ASSERT_EQ(status, Fw::Success::FAILURE);
    ASSERT_EVENTS_PortOpened_SIZE(0);

    // m_opened must have stayed false.
    U8 backing[4] = {0};
    Fw::Buffer buffer(backing, sizeof(backing));
    ASSERT_EQ(this->invoke_to_write(0, 0x50, buffer), Drv::I2cStatus::I2C_OPEN_ERR);
}

void Stm32I2cDriverTester ::testWriteBeforeOpen() {
    U8 backing[4] = {1, 2, 3, 4};
    Fw::Buffer buffer(backing, sizeof(backing));
    ASSERT_EQ(this->invoke_to_write(0, 0x50, buffer), Drv::I2cStatus::I2C_OPEN_ERR);
}

void Stm32I2cDriverTester ::testReadBeforeOpen() {
    U8 backing[4] = {0};
    Fw::Buffer buffer(backing, sizeof(backing));
    ASSERT_EQ(this->invoke_to_read(0, 0x50, buffer), Drv::I2cStatus::I2C_OPEN_ERR);
}

void Stm32I2cDriverTester ::testWriteReadBeforeOpen() {
    U8 wbacking[2] = {0};
    U8 rbacking[2] = {0};
    Fw::Buffer writeBuffer(wbacking, sizeof(wbacking));
    Fw::Buffer readBuffer(rbacking, sizeof(rbacking));
    ASSERT_EQ(this->invoke_to_writeRead(0, 0x50, writeBuffer, readBuffer), Drv::I2cStatus::I2C_OPEN_ERR);
}

void Stm32I2cDriverTester ::testWriteSuccess() {
    (void)this->component.open(I2cInstance::I2c1);

    U8 data[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    Fw::Buffer buffer(data, sizeof(data));
    const Drv::I2cStatus status = this->invoke_to_write(0, 0x50, buffer);
    ASSERT_EQ(status, Drv::I2cStatus::I2C_OK);
    ASSERT_EQ(Stub_lastDevAddress, 0x50);
    ASSERT_EQ(Stub_lastWriteLen, 4u);
    for (FwSizeType i = 0; i < 4; i++) {
        ASSERT_EQ(Stub_lastWriteData[i], data[i]);
    }
}

void Stm32I2cDriverTester ::testWriteFailure() {
    (void)this->component.open(I2cInstance::I2c1);
    Stub_hwMasterTransmitStatus = Drv::I2cStatus::I2C_ADDRESS_ERR;

    U8 data[1] = {0x00};
    Fw::Buffer buffer(data, sizeof(data));
    const Drv::I2cStatus status = this->invoke_to_write(0, 0x50, buffer);
    ASSERT_EQ(status, Drv::I2cStatus::I2C_ADDRESS_ERR);
}

void Stm32I2cDriverTester ::testWriteLargeBufferCapped() {
    (void)this->component.open(I2cInstance::I2c1);

    // Larger than Stub_lastWriteData's 32-byte capture buffer.
    U8 data[40];
    for (FwSizeType i = 0; i < sizeof(data); i++) {
        data[i] = static_cast<U8>(i);
    }
    Fw::Buffer buffer(data, sizeof(data));
    const Drv::I2cStatus status = this->invoke_to_write(0, 0x50, buffer);
    ASSERT_EQ(status, Drv::I2cStatus::I2C_OK);

    // The stub must truncate its capture rather than overrun its own
    // 32-byte buffer; the real HAL call underneath would see the full 40.
    ASSERT_EQ(Stub_lastWriteLen, 32u);
    for (FwSizeType i = 0; i < 32; i++) {
        ASSERT_EQ(Stub_lastWriteData[i], data[i]);
    }
}

void Stm32I2cDriverTester ::testReadSuccess() {
    (void)this->component.open(I2cInstance::I2c1);
    Stub_readResponseData[0] = 0x11;
    Stub_readResponseData[1] = 0x22;

    U8 backing[2] = {0};
    Fw::Buffer buffer(backing, sizeof(backing));
    const Drv::I2cStatus status = this->invoke_to_read(0, 0x50, buffer);
    ASSERT_EQ(status, Drv::I2cStatus::I2C_OK);
    ASSERT_EQ(Stub_lastDevAddress, 0x50);
    ASSERT_EQ(Stub_lastReadLen, 2u);
    ASSERT_EQ(backing[0], 0x11);
    ASSERT_EQ(backing[1], 0x22);
}

void Stm32I2cDriverTester ::testReadFailure() {
    (void)this->component.open(I2cInstance::I2c1);
    Stub_hwMasterReceiveStatus = Drv::I2cStatus::I2C_READ_ERR;

    U8 backing[1] = {0};
    Fw::Buffer buffer(backing, sizeof(backing));
    const Drv::I2cStatus status = this->invoke_to_read(0, 0x50, buffer);
    ASSERT_EQ(status, Drv::I2cStatus::I2C_READ_ERR);
}

void Stm32I2cDriverTester ::testReadLargeBufferCapped() {
    (void)this->component.open(I2cInstance::I2c1);
    for (FwSizeType i = 0; i < sizeof(Stub_readResponseData); i++) {
        Stub_readResponseData[i] = static_cast<U8>(0x40 + i);
    }

    // Larger than Stub_readResponseData's 32-byte canned response.
    U8 backing[40];
    for (FwSizeType i = 0; i < sizeof(backing); i++) {
        backing[i] = 0xAA;  // sentinel: bytes past the 32-byte response must stay untouched
    }
    Fw::Buffer buffer(backing, sizeof(backing));
    const Drv::I2cStatus status = this->invoke_to_read(0, 0x50, buffer);
    ASSERT_EQ(status, Drv::I2cStatus::I2C_OK);

    // The full requested length is still reported to the caller...
    ASSERT_EQ(Stub_lastReadLen, 40u);
    // ...but only the first 32 bytes were actually supplied by the stub.
    for (FwSizeType i = 0; i < 32; i++) {
        ASSERT_EQ(backing[i], Stub_readResponseData[i]);
    }
    for (FwSizeType i = 32; i < 40; i++) {
        ASSERT_EQ(backing[i], 0xAA);
    }
}

void Stm32I2cDriverTester ::testWriteReadSuccess() {
    (void)this->component.open(I2cInstance::I2c1);
    Stub_readResponseData[0] = 0x99;

    U8 wdata[1] = {0x10};  // e.g. a sensor register address
    U8 rbacking[1] = {0};
    Fw::Buffer writeBuffer(wdata, sizeof(wdata));
    Fw::Buffer readBuffer(rbacking, sizeof(rbacking));
    const Drv::I2cStatus status = this->invoke_to_writeRead(0, 0x50, writeBuffer, readBuffer);
    ASSERT_EQ(status, Drv::I2cStatus::I2C_OK);
    ASSERT_EQ(Stub_lastDevAddress, 0x50);
    ASSERT_EQ(Stub_lastWriteLen, 1u);
    ASSERT_EQ(Stub_lastWriteData[0], 0x10);
    ASSERT_EQ(Stub_lastReadLen, 1u);
    ASSERT_EQ(rbacking[0], 0x99);
}

void Stm32I2cDriverTester ::testWriteReadTransmitFailureShortCircuits() {
    (void)this->component.open(I2cInstance::I2c1);
    Stub_hwMasterTransmitStatus = Drv::I2cStatus::I2C_WRITE_ERR;
    Stub_lastReadLen = 999;  // sentinel: must stay untouched if hwMasterReceive never runs

    U8 wdata[1] = {0x10};
    U8 rbacking[1] = {0};
    Fw::Buffer writeBuffer(wdata, sizeof(wdata));
    Fw::Buffer readBuffer(rbacking, sizeof(rbacking));
    const Drv::I2cStatus status = this->invoke_to_writeRead(0, 0x50, writeBuffer, readBuffer);
    ASSERT_EQ(status, Drv::I2cStatus::I2C_WRITE_ERR);
    ASSERT_EQ(Stub_lastReadLen, 999u);
}

void Stm32I2cDriverTester ::testWriteReadReceiveFailure() {
    (void)this->component.open(I2cInstance::I2c1);
    Stub_hwMasterReceiveStatus = Drv::I2cStatus::I2C_READ_ERR;

    U8 wdata[1] = {0x10};
    U8 rbacking[1] = {0};
    Fw::Buffer writeBuffer(wdata, sizeof(wdata));
    Fw::Buffer readBuffer(rbacking, sizeof(rbacking));
    const Drv::I2cStatus status = this->invoke_to_writeRead(0, 0x50, writeBuffer, readBuffer);
    ASSERT_EQ(status, Drv::I2cStatus::I2C_READ_ERR);
    ASSERT_EQ(Stub_lastWriteLen, 1u);  // the write half did run before the failing read
}

}  // namespace Stm32
