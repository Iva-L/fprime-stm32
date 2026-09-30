// ======================================================================
// \title  Stm32GpioDriver.hpp
// \author ivanlara
// \brief  hpp file for the STM32H7 bare-metal GPIO pin driver
// ======================================================================

#ifndef STM32_GPIO_DRIVER_HPP
#define STM32_GPIO_DRIVER_HPP

#include <fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriverComponentAc.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/DirectionEnumAc.hpp>
#include <Fw/Types/SuccessEnumAc.hpp>
#include <Fw/Types/LogicEnumAc.hpp>
#include <Fw/Types/Assert.hpp>

namespace Stm32 {

//! GPIO port identifier. Kept HAL-free (no GPIO_TypeDef*) so this header has no vendor CMSIS/HAL dependency
enum class GpioPort { A, B, C, D, E, F, G, H, I, J, K };

class Stm32GpioDriver final : public Stm32GpioDriverComponentBase {
  public:

    //! Construct object Stm32GpioDriver
    explicit Stm32GpioDriver(const char* const compName);

    //! Destroy object Stm32GpioDriver
    ~Stm32GpioDriver();

    //! Enable the pin's GPIO port clock and configure the line as a push-pull
    //! output or a floating input. For an output pin, defaultState is applied
    //! before the mode switches to output, so the line never glitches through
    //! whatever level happened to be in the output register at reset.
    //! \param port: GPIO peripheral (e.g. GpioPort::F)
    //! \param pin: pin bit mask (e.g. GPIO_PIN_10)
    //! \param mode: OUTPUT or INPUT
    //! \param defaultState: initial level applied before enabling an OUTPUT pin
    Fw::Success open(GpioPort port, U16 pin, Fw::Direction mode, Fw::Logic defaultState = Fw::Logic::LOW);

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for gpioRead. Returns NOT_OPENED before open()
    //! and INVALID_MODE if the pin was configured as an output.
    Drv::GpioStatus gpioRead_handler(FwIndexType portNum, Fw::Logic& state) override;

    //! Handler implementation for gpioWrite. Returns NOT_OPENED before open()
    //! and INVALID_MODE if the pin was configured as an input.
    Drv::GpioStatus gpioWrite_handler(FwIndexType portNum, const Fw::Logic& state) override;

    //! Enable the port clock, apply defaultState (if OUTPUT), configure the
    //! pin, and verify the configuration actually took by reading back the
    //! mode register. Returns true if the readback matches what was requested.
    bool hwConfigurePin(GpioPort port, U16 pin, Fw::Direction mode, Fw::Logic defaultState);

    //! Read the current electrical level of an already-configured pin.
    bool hwReadPin(GpioPort port, U16 pin);

    //! Write the electrical level of an already-configured output pin.
    void hwWritePin(GpioPort port, U16 pin, bool high);

    GpioPort m_port;
    U16 m_pin;
    Fw::Direction m_mode;
    bool m_opened;
};

}  // namespace Stm32

#endif