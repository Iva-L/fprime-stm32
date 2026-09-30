// ======================================================================
// \title  Stm32GpioDriver.cpp
// \author ivanlara
// \brief  HAL boundary for the STM32 bare-metal GPIO pin driver (real
//         hardware implementation, STM32 targets only). This is the only
//         file in this driver allowed to include HAL headers -- see
//         Stm32GpioDriverStub.cpp for the host unit-test stand-in.
// ======================================================================

#include <fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriver.hpp>

// gpio.h is CubeMX's own always-present per-peripheral header (generated
// under this exact name for every STM32 family); it chains to the family
// HAL umbrella (via main.h), so including it instead of naming e.g.
// stm32h7xx_hal.h directly keeps this file portable across STM32 families.
#include "gpio.h"

namespace {

//! Map a HAL-free GpioPort identifier to the real CMSIS peripheral base.
GPIO_TypeDef* toHalPort(Stm32::GpioPort port) {
    switch (port) {
        case Stm32::GpioPort::A:
            return GPIOA;
        case Stm32::GpioPort::B:
            return GPIOB;
        case Stm32::GpioPort::C:
            return GPIOC;
        case Stm32::GpioPort::D:
            return GPIOD;
        case Stm32::GpioPort::E:
            return GPIOE;
        case Stm32::GpioPort::F:
            return GPIOF;
        case Stm32::GpioPort::G:
            return GPIOG;
        case Stm32::GpioPort::H:
            return GPIOH;
        case Stm32::GpioPort::I:
            return GPIOI;
        case Stm32::GpioPort::J:
            return GPIOJ;
        case Stm32::GpioPort::K:
            return GPIOK;
        default:
            FW_ASSERT(false, static_cast<FwAssertArgType>(port));
            return nullptr;
    }
}

//! Enable the AHB4 clock for whichever GPIO port is passed in
void enableGpioClock(GPIO_TypeDef* port) {
    FW_ASSERT(port != nullptr);

    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    } else if (port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    } else if (port == GPIOF) {
        __HAL_RCC_GPIOF_CLK_ENABLE();
    } else if (port == GPIOG) {
        __HAL_RCC_GPIOG_CLK_ENABLE();
    } else if (port == GPIOH) {
        __HAL_RCC_GPIOH_CLK_ENABLE();
    } else if (port == GPIOI) {
        __HAL_RCC_GPIOI_CLK_ENABLE();
    } else if (port == GPIOJ) {
        __HAL_RCC_GPIOJ_CLK_ENABLE();
    } else if (port == GPIOK) {
        __HAL_RCC_GPIOK_CLK_ENABLE();
    } else {
        FW_ASSERT(false, reinterpret_cast<FwAssertArgType>(port));
    }
}

}  // namespace

namespace Stm32 {

bool Stm32GpioDriver ::hwConfigurePin(GpioPort port, U16 pin, Fw::Direction mode, Fw::Logic defaultState) {
    GPIO_TypeDef* halPort = toHalPort(port);

    FW_ASSERT(halPort != nullptr);

    enableGpioClock(halPort);

    if (mode == Fw::Direction::OUT) {
        HAL_GPIO_WritePin(halPort, pin, (defaultState == Fw::Logic::HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    GPIO_InitTypeDef init = {};
    init.Pin = pin;
    init.Mode = (mode == Fw::Direction::OUT) ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT;
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(halPort, &init);

    // Verify that the GPIO was successfully configured for the specified mode.
    const U32 position = POSITION_VAL(pin);
    const U32 actualModeBits = (halPort->MODER >> (position * 2U)) & GPIO_MODE;
    const U32 expectedModeBits = init.Mode & GPIO_MODE;
    return actualModeBits == expectedModeBits;
}

bool Stm32GpioDriver ::hwReadPin(GpioPort port, U16 pin) {
    return HAL_GPIO_ReadPin(toHalPort(port), pin) == GPIO_PIN_SET;
}

void Stm32GpioDriver ::hwWritePin(GpioPort port, U16 pin, bool high) {
    HAL_GPIO_WritePin(toHalPort(port), pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

}  // namespace Stm32