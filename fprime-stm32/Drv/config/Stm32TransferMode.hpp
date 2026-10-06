// ======================================================================
// \title  Stm32TransferMode.hpp
// \author ivanlara
// \brief  Shared runtime transfer-mode selector for the STM32 SPI/I2C/UART
//         drivers, so each component instance (not each component class)
//         picks polled vs. DMA at open() time.
// ======================================================================
#ifndef STM32_TRANSFER_MODE_HPP
#define STM32_TRANSFER_MODE_HPP

namespace Stm32 {

//! Per-instance transfer strategy, selected at open() time.
enum class TransferMode {
    POLLED,  //!< Direct CPU-driven blocking transfer. Safe from any RAM region (DTCM included).
    DMA      //!< Hardware DMA engine transfer. Buffers must live outside DTCM (see CacheMaintenance.hpp).
};

}  // namespace Stm32

#endif  // STM32_TRANSFER_MODE_HPP
