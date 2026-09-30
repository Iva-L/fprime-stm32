// ======================================================================
// \title  Stm32GpioDriverTester.hpp
// \author ivanlara
// \brief  hpp file for Stm32GpioDriver component test harness implementation class
// ======================================================================

#ifndef Stm32_Stm32GpioDriverTester_HPP
#define Stm32_Stm32GpioDriverTester_HPP

#include "fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriver.hpp"
#include "fprime-stm32/Drv/STM32GpioDriver/Stm32GpioDriverGTestBase.hpp"

namespace Stm32 {

class Stm32GpioDriverTester final : public Stm32GpioDriverGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 10;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object Stm32GpioDriverTester
    Stm32GpioDriverTester();

    //! Destroy object Stm32GpioDriverTester
    ~Stm32GpioDriverTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! open() as an OUTPUT succeeds and emits ConfigureSuccess
    void testOpenOutputSuccess();

    //! open() as an INPUT succeeds and emits ConfigureSuccess
    void testOpenInputSuccess();

    //! open() reports ConfigureError and leaves the driver unopened when the
    //! HAL boundary reports the mode readback did not match (hwConfigurePin
    //! failure injected via Stub_hwConfigurePin)
    void testOpenFailure();

    //! gpioWrite/gpioRead before open() both report NOT_OPENED
    void testAccessBeforeOpen();

    //! gpioWrite on a pin opened as INPUT reports INVALID_MODE
    void testWriteWrongMode();

    //! gpioRead on a pin opened as OUTPUT reports INVALID_MODE
    void testReadWrongMode();

    //! gpioWrite on a pin opened as OUTPUT reports OP_OK and drives the HAL
    //! boundary with the correct level, observable via Stub_lastWrittenPinHigh
    void testWriteAfterOpen();

    //! gpioRead on a pin opened as INPUT reports OP_OK and returns the level
    //! injected via Stub_hwReadPin (both LOW and HIGH)
    void testReadAfterOpen();

  private:
    // ----------------------------------------------------------------------
    // Test support
    // ----------------------------------------------------------------------

    //! Reset every Stub_* global back to its documented default so each test
    //! starts from a known, hermetic state.
    void resetStubState();

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    Stm32GpioDriver component;
};

}  // namespace Stm32

#endif
