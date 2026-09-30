// ======================================================================
// \title  Stm32GpioDriverCommon.cpp
// \author ivanlara
// \brief  Hardware-independent logic for the STM32H7 bare-metal GPIO pin
//         driver: shared by the real (stm32h7) and stubbed (host UT)
//         builds. Contains no HAL/CMSIS dependency.
// ======================================================================

#include <fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriver.hpp>

namespace Stm32 {

Stm32GpioDriver ::Stm32GpioDriver(const char* const compName)
    : Stm32GpioDriverComponentBase(compName),
      m_port(GpioPort::A),
      m_pin(0),
      m_mode(Fw::Direction::IN),
      m_opened(false) {}

Stm32GpioDriver ::~Stm32GpioDriver() {}

Fw::Success Stm32GpioDriver ::open(GpioPort port, U16 pin, Fw::Direction mode, Fw::Logic defaultState) {
    if (!this->hwConfigurePin(port, pin, mode, defaultState)) {
        this->log_WARNING_HI_ConfigureError(pin, mode);
        this->m_opened = false;
        return Fw::Success::FAILURE;
    }

    this->log_ACTIVITY_HI_ConfigureSuccess(pin, mode);
    this->m_port = port;
    this->m_pin = pin;
    this->m_mode = mode;
    this->m_opened = true;
    return Fw::Success::SUCCESS;
}

Drv::GpioStatus Stm32GpioDriver ::gpioRead_handler(FwIndexType portNum, Fw::Logic& state) {
    if (!this->m_opened) {
        return Drv::GpioStatus::NOT_OPENED;
    }
    if (this->m_mode != Fw::Direction::IN) {
        return Drv::GpioStatus::INVALID_MODE;
    }
    state = this->hwReadPin(this->m_port, this->m_pin) ? Fw::Logic::HIGH : Fw::Logic::LOW;
    return Drv::GpioStatus::OP_OK;
}

Drv::GpioStatus Stm32GpioDriver ::gpioWrite_handler(FwIndexType portNum, const Fw::Logic& state) {
    if (!this->m_opened) {
        return Drv::GpioStatus::NOT_OPENED;
    }
    if (this->m_mode != Fw::Direction::OUT) {
        return Drv::GpioStatus::INVALID_MODE;
    }
    this->hwWritePin(this->m_port, this->m_pin, state == Fw::Logic::HIGH);
    return Drv::GpioStatus::OP_OK;
}

}  // namespace Stm32