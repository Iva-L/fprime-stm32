// ======================================================================
// \title  STM32SpiDriver.hpp
// \author ivanlara
// \brief  hpp file for STM32SpiDriver component implementation class
// ======================================================================

#ifndef Stm32_STM32SpiDriver_HPP
#define Stm32_STM32SpiDriver_HPP

#include "fprime-stm32/Drv/STM32SpiDriver/STM32SpiDriverComponentAc.hpp"

namespace Stm32 {

class STM32SpiDriver final : public STM32SpiDriverComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct STM32SpiDriver object
    STM32SpiDriver(const char* const compName  //!< The component name
    );

    //! Destroy STM32SpiDriver object
    ~STM32SpiDriver();
};

}  // namespace Stm32

#endif
