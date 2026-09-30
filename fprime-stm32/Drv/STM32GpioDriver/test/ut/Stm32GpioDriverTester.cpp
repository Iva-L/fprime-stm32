// ======================================================================
// \title  Stm32GpioDriverTester.cpp
// \author ivanlara
// \brief  cpp file for Stm32GpioDriver component test harness implementation class
// ======================================================================

#include "Stm32GpioDriverTester.hpp"

// Stub_* globals defined in Stm32GpioDriverStub.cpp
extern bool Stub_hwConfigurePin;
extern bool Stub_hwReadPin;
extern bool Stub_lastWrittenPinHigh;
extern U32 Stub_hwWriteCallCount;

namespace Stm32 {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

Stm32GpioDriverTester ::Stm32GpioDriverTester()
    : Stm32GpioDriverGTestBase("Stm32GpioDriverTester", Stm32GpioDriverTester::MAX_HISTORY_SIZE),
      component("Stm32GpioDriver") {
    this->resetStubState();
    this->initComponents();
    this->connectPorts();
}

Stm32GpioDriverTester ::~Stm32GpioDriverTester() {
    this->component.deinit();
}

void Stm32GpioDriverTester ::resetStubState() {
    Stub_hwConfigurePin = true;
    Stub_hwReadPin = false;
    Stub_lastWrittenPinHigh = false;
    Stub_hwWriteCallCount = 0;
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void Stm32GpioDriverTester ::testOpenOutputSuccess() {
    const Fw::Success status = this->component.open(GpioPort::F, 10, Fw::Direction::OUT, Fw::Logic::LOW);
    ASSERT_EQ(status, Fw::Success::SUCCESS);
    ASSERT_EVENTS_ConfigureSuccess_SIZE(1);
    ASSERT_EVENTS_ConfigureSuccess(0, 10, Fw::Direction::OUT);
    ASSERT_EVENTS_ConfigureError_SIZE(0);
}

void Stm32GpioDriverTester ::testOpenInputSuccess() {
    const Fw::Success status = this->component.open(GpioPort::A, 3, Fw::Direction::IN);
    ASSERT_EQ(status, Fw::Success::SUCCESS);
    ASSERT_EVENTS_ConfigureSuccess_SIZE(1);
    ASSERT_EVENTS_ConfigureSuccess(0, 3, Fw::Direction::IN);
}

void Stm32GpioDriverTester ::testOpenFailure() {
    Stub_hwConfigurePin = false;  // simulate a MODER readback mismatch

    const Fw::Success status = this->component.open(GpioPort::F, 10, Fw::Direction::OUT);
    ASSERT_EQ(status, Fw::Success::FAILURE);
    ASSERT_EVENTS_ConfigureError_SIZE(1);
    ASSERT_EVENTS_ConfigureError(0, 10, Fw::Direction::OUT);
    ASSERT_EVENTS_ConfigureSuccess_SIZE(0);

    // m_opened must have stayed false: neither port is usable after a
    // failed open(), regardless of the mode that was requested.
    const Drv::GpioStatus writeStatus = this->invoke_to_gpioWrite(0, Fw::Logic::HIGH);
    ASSERT_EQ(writeStatus, Drv::GpioStatus::NOT_OPENED);

    Fw::Logic state = Fw::Logic::LOW;
    const Drv::GpioStatus readStatus = this->invoke_to_gpioRead(0, state);
    ASSERT_EQ(readStatus, Drv::GpioStatus::NOT_OPENED);
}

void Stm32GpioDriverTester ::testAccessBeforeOpen() {
    const Drv::GpioStatus writeStatus = this->invoke_to_gpioWrite(0, Fw::Logic::HIGH);
    ASSERT_EQ(writeStatus, Drv::GpioStatus::NOT_OPENED);

    Fw::Logic state = Fw::Logic::LOW;
    const Drv::GpioStatus readStatus = this->invoke_to_gpioRead(0, state);
    ASSERT_EQ(readStatus, Drv::GpioStatus::NOT_OPENED);
}

void Stm32GpioDriverTester ::testWriteWrongMode() {
    (void)this->component.open(GpioPort::F, 10, Fw::Direction::IN);
    const Drv::GpioStatus status = this->invoke_to_gpioWrite(0, Fw::Logic::HIGH);
    ASSERT_EQ(status, Drv::GpioStatus::INVALID_MODE);
}

void Stm32GpioDriverTester ::testReadWrongMode() {
    (void)this->component.open(GpioPort::F, 10, Fw::Direction::OUT);
    Fw::Logic state = Fw::Logic::LOW;
    const Drv::GpioStatus status = this->invoke_to_gpioRead(0, state);
    ASSERT_EQ(status, Drv::GpioStatus::INVALID_MODE);
}

void Stm32GpioDriverTester ::testWriteAfterOpen() {
    (void)this->component.open(GpioPort::F, 10, Fw::Direction::OUT);

    Drv::GpioStatus status = this->invoke_to_gpioWrite(0, Fw::Logic::HIGH);
    ASSERT_EQ(status, Drv::GpioStatus::OP_OK);
    ASSERT_EQ(Stub_lastWrittenPinHigh, true);
    ASSERT_EQ(Stub_hwWriteCallCount, 1u);

    status = this->invoke_to_gpioWrite(0, Fw::Logic::LOW);
    ASSERT_EQ(status, Drv::GpioStatus::OP_OK);
    ASSERT_EQ(Stub_lastWrittenPinHigh, false);
    ASSERT_EQ(Stub_hwWriteCallCount, 2u);
}

void Stm32GpioDriverTester ::testReadAfterOpen() {
    (void)this->component.open(GpioPort::A, 3, Fw::Direction::IN);

    Stub_hwReadPin = false;
    Fw::Logic state = Fw::Logic::HIGH;  // deliberately wrong, so a bad read_handler can't pass by accident
    Drv::GpioStatus status = this->invoke_to_gpioRead(0, state);
    ASSERT_EQ(status, Drv::GpioStatus::OP_OK);
    ASSERT_EQ(state, Fw::Logic::LOW);

    Stub_hwReadPin = true;
    state = Fw::Logic::LOW;
    status = this->invoke_to_gpioRead(0, state);
    ASSERT_EQ(status, Drv::GpioStatus::OP_OK);
    ASSERT_EQ(state, Fw::Logic::HIGH);
}

}  // namespace Stm32
