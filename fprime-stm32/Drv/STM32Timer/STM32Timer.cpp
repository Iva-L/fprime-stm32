// ======================================================================
// \title  STM32Timer.cpp
// \author ivanlara
// \brief  HAL boundary for the STM32H7 TIM CH2 tick source (real
//         hardware implementation, stm32h7 target only). This is the only
//         file in this driver allowed to include tim.h -- see
//         STM32TimerStub.cpp for the host unit-test stand-in.
// ======================================================================

#include <fprime-stm32/Drv/STM32Timer/STM32Timer.hpp>

#include "tim.h"

namespace {

//! Callback registry: the ISR callback below is a free function with no
//! user-context pointer, only a raw TIM_HandleTypeDef*, so mapping back to
//! "which STM32Timer instance owns this handle" needs a table rather than
//! a single cached pointer -- that's what lets multiple instances
//! (different physical timers) be open at the same time. Indexed by
//! TimerInstance's own cardinality, matching the "named capacity constant"
//! convention already used elsewhere in this codebase (e.g.
//! Os::Baremetal::TaskRunner::TASK_CAPACITY).
constexpr FwSizeType TIMER_INSTANCE_CAPACITY = 14;

struct TimerRegistryEntry {
    TIM_HandleTypeDef* handle = nullptr;
    Stm32::STM32Timer* component = nullptr;
};

TimerRegistryEntry s_timerRegistry[TIMER_INSTANCE_CAPACITY];

//! Convert a HAL-free TimerInstance to the corresponding HAL handle.
TIM_HandleTypeDef* toHalHandle(Stm32::TimerInstance instance) {

    TIM_HandleTypeDef* deviceHandle = nullptr;
    
    switch (instance) {
        case Stm32::TimerInstance::Tim1:
            #if TIM1_INSTANCE
            deviceHandle = &htim1;
            #endif
            break;
        case Stm32::TimerInstance::Tim2:
            #if TIM2_INSTANCE
            deviceHandle = &htim2;
            #endif
            break;
        case Stm32::TimerInstance::Tim3:
            #if TIM3_INSTANCE
            deviceHandle = &htim3;
            #endif
            break;
        case Stm32::TimerInstance::Tim4:
            #if TIM4_INSTANCE
            deviceHandle = &htim4;
            #endif
            break;
        case Stm32::TimerInstance::Tim5:
            #if TIM5_INSTANCE
            deviceHandle = &htim5;
            #endif
            break;
        case Stm32::TimerInstance::Tim6:
            #if TIM6_INSTANCE
            deviceHandle = &htim6;
            #endif
            break;
        case Stm32::TimerInstance::Tim7:
            #if TIM7_INSTANCE
            deviceHandle = &htim7;
            #endif
            break;
        case Stm32::TimerInstance::Tim8:
            #if TIM8_INSTANCE
            deviceHandle = &htim8;
            #endif
            break;
        case Stm32::TimerInstance::Tim12:
            #if TIM12_INSTANCE
            deviceHandle = &htim12;
            #endif
            break;
        case Stm32::TimerInstance::Tim13:
            #if TIM13_INSTANCE
            deviceHandle = &htim13;
            #endif
            break;
        case Stm32::TimerInstance::Tim14:
            #if TIM14_INSTANCE
            deviceHandle = &htim14;
            #endif
            break;
        case Stm32::TimerInstance::Tim15:
            #if TIM15_INSTANCE
            deviceHandle = &htim15;
            #endif
            break;
        case Stm32::TimerInstance::Tim16:
            #if TIM16_INSTANCE
            deviceHandle = &htim16;
            #endif
            break;
        case Stm32::TimerInstance::Tim17:
            #if TIM17_INSTANCE
            deviceHandle = &htim17;
            #endif
            break;
        default:
            FW_ASSERT(false, static_cast<FwAssertArgType>(instance));
            break;
    }
    return deviceHandle;
}

//! Look up which live STM32Timer instance (if any) owns `htim`, by linear
//! scan of the small (<=14-entry) registry. Negligible ISR cost, and the
//! only way to resolve identity: the HAL calls this callback with no
//! instance/context argument, only the raw handle pointer.
Stm32::STM32Timer* findTimerComponent(TIM_HandleTypeDef* htim) {
    for (FwSizeType i = 0; i < TIMER_INSTANCE_CAPACITY; i++) {
        if (s_timerRegistry[i].handle == htim && s_timerRegistry[i].component != nullptr) {
            return s_timerRegistry[i].component;
        }
    }
    return nullptr;
}

}  // namespace

extern "C" void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef* htim) {
    Stm32::STM32Timer* const component = findTimerComponent(htim);
    if (component != nullptr) {
        component->signalTick();
    }
}

namespace Stm32 {

void STM32Timer ::hwSelectInstance(TimerInstance instance) {
    TIM_HandleTypeDef* const halHandle = toHalHandle(instance);
    FW_ASSERT(halHandle != nullptr, static_cast<FwAssertArgType>(instance));

    // Registered by instance index (bounds-safe: the enum itself can't
    // index outside the table), not a single shared pointer -- this is
    // what makes a second, simultaneously-open instance safe.
    FW_ASSERT(static_cast<FwSizeType>(instance) < TIMER_INSTANCE_CAPACITY,
              static_cast<FwAssertArgType>(instance));
    s_timerRegistry[static_cast<FwSizeType>(instance)] = {halHandle, this};

    this->m_instance = instance;
}

void STM32Timer ::hwArmChannel(U32 target) {
    TIM_HandleTypeDef* const halHandle = toHalHandle(this->m_instance);
    FW_ASSERT(halHandle != nullptr);

    TIM_OC_InitTypeDef ocConfig = {};
    ocConfig.OCMode = TIM_OCMODE_TIMING;  // "Frozen": compare-match interrupt only, no pin/output effect
    ocConfig.Pulse = target;
    ocConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    ocConfig.OCFastMode = TIM_OCFAST_DISABLE;

    HAL_StatusTypeDef status = HAL_TIM_OC_ConfigChannel(halHandle, &ocConfig, TIM_CHANNEL_2);
    FW_ASSERT(status == HAL_OK, static_cast<FwAssertArgType>(status));

    status = HAL_TIM_OC_Start_IT(halHandle, TIM_CHANNEL_2);
    FW_ASSERT(status == HAL_OK, static_cast<FwAssertArgType>(status));
}

U32 STM32Timer ::hwReadCounter() {
    TIM_HandleTypeDef* const halHandle = toHalHandle(this->m_instance);
    FW_ASSERT(halHandle != nullptr);
    return halHandle->Instance->CNT;
}

void STM32Timer ::hwSetCompare(U32 target) {
    TIM_HandleTypeDef* const halHandle = toHalHandle(this->m_instance);
    FW_ASSERT(halHandle != nullptr);
    __HAL_TIM_SET_COMPARE(halHandle, TIM_CHANNEL_2, target);
}

}  // namespace Stm32
