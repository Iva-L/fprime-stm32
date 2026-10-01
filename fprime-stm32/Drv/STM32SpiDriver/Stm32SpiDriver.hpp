// ======================================================================
// \title  Stm32SpiDriver.hpp
// \author ivanlara
// \brief  hpp file for the STM32H7 SPI blocking/polled master driver
// ======================================================================

#ifndef Stm32_Stm32SpiDriver_HPP
#define Stm32_Stm32SpiDriver_HPP

#include "fprime-stm32/Drv/STM32SpiDriver/Stm32SpiDriverComponentAc.hpp"
#include "fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriver.hpp"
#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/SuccessEnumAc.hpp>
#include <Fw/Types/Assert.hpp>
#include <config/Stm32Config.hpp>

namespace Stm32 {

//! SPI peripheral identifier, covering every instance present on an STM32H753.
//! Only Spi5 has a CubeMX-generated MX_SPIn_Init()/handle in this project
//! today (see Hardware/stm32h753_hal/Core/Src/spi.c) -- the others are
//! declared so a different board/peripheral selection only requires adding
//! that CubeMX config and one switch case in Stm32SpiDriver.cpp, not
//! touching this header, Common.cpp, or the Stub.
enum class SpiInstance { Spi1, Spi2, Spi3, Spi4, Spi5, Spi6 };

class Stm32SpiDriver final : public Stm32SpiDriverComponentBase {
  public:
    //! Construct Stm32SpiDriver object
    Stm32SpiDriver(const char* const compName  //!< The component name
    );

    //! Destroy Stm32SpiDriver object
    ~Stm32SpiDriver();

    //! Configure the selected SPI instance (via the CubeMX-generated
    //! MX_SPIn_Init()) and the chip-select GPIO pin this instance drives
    //! around every transfer (SPI5 is configured NSS_SOFT -- the HAL never
    //! toggles a CS pin on its own), then mark the driver ready to accept
    //! transactions. Must be called once from configureTopology(), before
    //! any SpiWriteRead/SpiReadWrite port invocation.
    //! \param instance which SPI peripheral this driver instance owns
    //! \param csPort GPIO port of the chip-select pin this instance drives
    //! \param csPin GPIO pin number of the chip-select pin
    //! \param timeoutMs per-transaction blocking watchdog passed to every
    //!        HAL_SPI_TransmitReceive() call
    Fw::Success open(SpiInstance instance, Stm32::GpioPort csPort, U16 csPin, U32 timeoutMs = 10);

  private:
    //! Drive the chip-select pin. `active` selects the device (CS low);
    //! !active idles the bus (CS high). Re-resolves the HAL GPIO port from
    //! m_csPort on every call rather than caching a HAL pointer, so that
    //! multiple Stm32SpiDriver instances (different bus + CS per sensor)
    //! never share any HAL-typed state.
    void hwSetCs(bool active);

    //! Blocking (polled) full-duplex transfer of `size` bytes, bounded by
    //! m_timeoutMs. Returns the raw HAL_StatusTypeDef value (0 == HAL_OK);
    //! does not toggle chip-select -- callers bracket this with hwSetCs().
    //! Re-resolves the HAL SPI handle from m_instance on every call, for
    //! the same multi-instance-safety reason as hwSetCs().
    I32 hwTransmitReceive(const U8* txData, U8* rxData, FwSizeType size);

    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Guarded synchronous full-duplex transfer. Returns SPI_OPEN_ERR before open().
    Drv::SpiStatus SpiWriteRead_handler(FwIndexType portNum, Fw::Buffer& writeBuffer, Fw::Buffer& readBuffer) override;

    //! DEPRECATED synchronous full-duplex transfer (same operation, no return value).
    //! Forwards to SpiWriteRead_handler() and discards the status, matching
    //! Drv::LinuxSpiDriver's convention for this deprecated port.
    void SpiReadWrite_handler(FwIndexType portNum, Fw::Buffer& writeBuffer, Fw::Buffer& readBuffer) override;

    SpiInstance m_instance;
    Stm32::GpioPort m_csPort;
    U16 m_csPin;
    U32 m_timeoutMs;
    bool m_opened;
};

}  // namespace Stm32

#endif  // Stm32_Stm32SpiDriver_HPP
