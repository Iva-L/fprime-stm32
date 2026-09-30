// ======================================================================
// \title  Stm32SpiDriverCommon.cpp
// \author ivanlara
// \brief  Hardware-independent port handler logic for the STM32H7 SPI
//         blocking master driver: shared by the real (stm32h7) and stubbed
//         (host UT) builds. Contains no HAL/CMSIS dependency -- every
//         register touch is behind hwSetCs()/hwTransmitReceive() (declared
//         in Stm32SpiDriver.hpp) or open() itself, implemented in
//         Stm32SpiDriver.cpp (real) / Stm32SpiDriverStub.cpp (host).
// ======================================================================

#include <fprime-stm32/Drv/STM32SpiDriver/Stm32SpiDriver.hpp>
#include <Fw/Types/Assert.hpp>

#if SPI_ENABLED

namespace Stm32 {

// ----------------------------------------------------------------------
// Construction, initialization, and destruction
// ----------------------------------------------------------------------

Stm32SpiDriver ::Stm32SpiDriver(const char* const compName)
    : Stm32SpiDriverComponentBase(compName),
      m_instance(SpiInstance::Spi5),
      m_csPort(Stm32::GpioPort::A),
      m_csPin(0),
      m_timeoutMs(10),
      m_opened(false) {}

Stm32SpiDriver ::~Stm32SpiDriver() {}

// ----------------------------------------------------------------------
// Handler implementations for user-defined typed input ports
// ----------------------------------------------------------------------

// Every sensor payload on this bus is small (6-8 bytes for the BMP280), so a
// single blocking HAL_SPI_TransmitReceive() bracketed by chip-select is
// mandatory-simple here -- no DMA, no interrupts, no cache-coherence or
// AXI SRAM alignment concerns (see docs/sdd.md).
Drv::SpiStatus Stm32SpiDriver ::SpiWriteRead_handler(FwIndexType portNum, Fw::Buffer& writeBuffer, Fw::Buffer& readBuffer) {
    if (!this->m_opened) {
        return Drv::SpiStatus::SPI_OPEN_ERR;
    }
    FW_ASSERT(writeBuffer.getData() != nullptr);
    FW_ASSERT(readBuffer.getData() != nullptr);
    FW_ASSERT(writeBuffer.getSize() == readBuffer.getSize(), static_cast<FwAssertArgType>(writeBuffer.getSize()),
              static_cast<FwAssertArgType>(readBuffer.getSize()));

    this->hwSetCs(true);
    const I32 status = this->hwTransmitReceive(writeBuffer.getData(), readBuffer.getData(), writeBuffer.getSize());
    this->hwSetCs(false);

    if (status != 0) {  // 0 == HAL_OK
        Fw::LogStringArg _op("TransmitReceive");
        this->log_WARNING_HI_HalError(_op, status);
        return Drv::SpiStatus::SPI_WRITE_ERR;
    }
    return Drv::SpiStatus::SPI_OK;
}

void Stm32SpiDriver ::SpiReadWrite_handler(FwIndexType portNum, Fw::Buffer& writeBuffer, Fw::Buffer& readBuffer) {
    (void)this->SpiWriteRead_handler(portNum, writeBuffer, readBuffer);
}

}  // namespace Stm32

#endif  // SPI_ENABLED
