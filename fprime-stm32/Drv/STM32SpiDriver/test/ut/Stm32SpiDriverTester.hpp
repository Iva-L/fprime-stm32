// ======================================================================
// \title  Stm32SpiDriverTester.hpp
// \author ivanlara
// \brief  hpp file for Stm32SpiDriver component test harness implementation class
// ======================================================================

#ifndef Stm32_Stm32SpiDriverTester_HPP
#define Stm32_Stm32SpiDriverTester_HPP

#include "fprime-stm32/Drv/STM32SpiDriver/Stm32SpiDriver.hpp"
#include "fprime-stm32/Drv/STM32SpiDriver/Stm32SpiDriverGTestBase.hpp"

namespace Stm32 {

class Stm32SpiDriverTester final : public Stm32SpiDriverGTestBase {
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

    //! Construct object Stm32SpiDriverTester
    Stm32SpiDriverTester();

    //! Destroy object Stm32SpiDriverTester
    ~Stm32SpiDriverTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! open() succeeds, forwards instance/CS to the HAL boundary, and emits PortOpened
    void testOpenSuccess();

    //! open() reports FAILURE and leaves the driver unopened when the HAL
    //! boundary's init fails (injected via Stub_hwOpenSucceeds)
    void testOpenFailure();

    //! open() without an explicit mode argument defaults to POLLED (backward compatibility)
    void testOpenDefaultsToPolled();

    //! open() forwards an explicit DMA mode request to the HAL boundary
    void testOpenDma();

    //! SpiWriteRead() in DMA mode performs the same logical transfer as
    //! POLLED (CS bracketing, exact tx/rx bytes) once the completion
    //! callback fires
    void testSpiWriteReadDmaSuccess();

    //! SpiWriteRead() in DMA mode propagates a HAL boundary failure status
    //! reported after DMA "completion" and emits HalError
    void testSpiWriteReadDmaFailure();

    //! SpiWriteRead() in DMA mode times out and reports SPI_WRITE_ERR when
    //! the completion callback never fires (injected via Stub_dmaCompletes)
    void testSpiWriteReadDmaTimeout();

    //! Re-opening the same component instance with a different mode switches
    //! its transfer behavior -- proves TransferMode is per-open() instance
    //! state, not shared/global
    void testReopenSwitchesMode();

    //! SpiWriteRead()/SpiReadWrite() before open() report/return SPI_OPEN_ERR
    void testSpiWriteReadBeforeOpen();
    void testSpiReadWriteBeforeOpen();

    //! SpiWriteRead() asserts CS active for the duration of the transfer and
    //! forwards the exact write bytes / returns the exact read bytes
    void testSpiWriteReadSuccess();

    //! SpiWriteRead() propagates a HAL boundary failure status and emits HalError
    void testSpiWriteReadFailure();

    //! SpiReadWrite() performs the same transfer as SpiWriteRead() but discards the status
    void testSpiReadWriteSuccess();

    //! A write buffer larger than the stub's capture buffer is truncated, not overrun
    void testSpiWriteReadLargeBufferCapped();

  private:
    // ----------------------------------------------------------------------
    // Test support
    // ----------------------------------------------------------------------

    //! Reset every Stub_* global back to its documented default so each
    //! test starts from a known, hermetic state regardless of run order --
    //! these are shared, process-wide globals (see Stm32SpiDriverStub.cpp),
    //! not per-Tester state.
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
    Stm32SpiDriver component;
};

}  // namespace Stm32

#endif
