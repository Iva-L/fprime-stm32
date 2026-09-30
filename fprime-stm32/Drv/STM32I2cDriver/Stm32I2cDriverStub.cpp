// ======================================================================
// \title  Stm32I2cDriverStub.cpp
// \brief  HAL boundary stand-in for host-native unit tests. No I2C1
//         hardware exists on the build host, so every call here operates
//         on injectable/observable stub state instead of real registers --
//         no HAL/CMSIS include, no register access. A unit test drives
//         success/failure via the extern stub state below instead of
//         relying on real HAL_I2C_* return codes.
// ======================================================================

#include <fprime-stm32/Drv/STM32I2cDriver/Stm32I2cDriver.hpp>

// Injectable stub state for unit tests
extern bool Stub_hwOpenSucceeds = true;                                    // simulates MX_I2Cn_Init()/speed-override HAL_I2C_Init()
extern Drv::I2cStatus Stub_hwMasterTransmitStatus = Drv::I2cStatus::I2C_OK;  // status hwMasterTransmit() reports
extern Drv::I2cStatus Stub_hwMasterReceiveStatus = Drv::I2cStatus::I2C_OK;   // status hwMasterReceive() reports
extern U8 Stub_readResponseData[32] = {0};        // bytes hwMasterReceive() copies into the caller's buffer

// Observable stub state for unit tests
extern Stm32::I2cInstance Stub_lastOpenedInstance = Stm32::I2cInstance::I2c1;  // instance most recently passed to open()
extern Stm32::I2cBusSpeed Stub_lastRequestedBusSpeed = Stm32::I2cBusSpeed::Fast;  // busSpeed most recently passed to open()
extern U16 Stub_lastDevAddress = 0;
extern U8 Stub_lastWriteData[32] = {0};
extern U16 Stub_lastWriteLen = 0;
extern U16 Stub_lastReadLen = 0;

namespace Stm32 {

Fw::Success Stm32I2cDriver ::open(I2cInstance instance, I2cBusSpeed busSpeed) {
    Stub_lastOpenedInstance = instance;
    Stub_lastRequestedBusSpeed = busSpeed;
    if (!Stub_hwOpenSucceeds) {
        return Fw::Success::FAILURE;
    }
    this->m_instance = instance;
    this->m_opened = true;
    Fw::LogStringArg _speedArg(busSpeed == Stm32::I2cBusSpeed::Standard ? "Standard" :
                               busSpeed == Stm32::I2cBusSpeed::Fast ? "Fast" :
                               busSpeed == Stm32::I2cBusSpeed::FastPlus ? "FastPlus" : "Unknown");

    this->log_ACTIVITY_HI_PortOpened(_speedArg);
    return Fw::Success::SUCCESS;
}

Drv::I2cStatus Stm32I2cDriver ::hwMasterTransmit(U16 devAddress, U8* data, U16 len) {
    Stub_lastDevAddress = devAddress;

    const FwSizeType captured =
        (static_cast<FwSizeType>(len) < sizeof(Stub_lastWriteData)) ? static_cast<FwSizeType>(len) : sizeof(Stub_lastWriteData);
    for (FwSizeType i = 0; i < captured; i++) {
        Stub_lastWriteData[i] = data[i];
    }
    Stub_lastWriteLen = static_cast<U16>(captured);

    return Stub_hwMasterTransmitStatus;
}

Drv::I2cStatus Stm32I2cDriver ::hwMasterReceive(U16 devAddress, U8* data, U16 len) {
    Stub_lastDevAddress = devAddress;
    Stub_lastReadLen = len;

    const FwSizeType toCopy =
        (static_cast<FwSizeType>(len) < sizeof(Stub_readResponseData)) ? static_cast<FwSizeType>(len) : sizeof(Stub_readResponseData);
    for (FwSizeType i = 0; i < toCopy; i++) {
        data[i] = Stub_readResponseData[i];
    }

    return Stub_hwMasterReceiveStatus;
}

}  // namespace Stm32
