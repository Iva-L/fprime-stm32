// ======================================================================
// \title  BootstrapAllocator.hpp
// \brief  Fixed-pool allocator used during deployment initialization
// ======================================================================
#ifndef FPRIME_STM32_BOOTSTRAPALLOCATOR_HPP
#define FPRIME_STM32_BOOTSTRAPALLOCATOR_HPP

#include <Fw/Types/MemAllocator.hpp>

namespace Stm32 {

Fw::MemAllocator& getBootstrapAllocator();
void lockBootstrapAllocator();

}  // namespace Stm32

extern "C" void Stm32_registerBootstrapAllocator();

#endif  // FPRIME_STM32_BOOTSTRAPALLOCATOR_HPP
