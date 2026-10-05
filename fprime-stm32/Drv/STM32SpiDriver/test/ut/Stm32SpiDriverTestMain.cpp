// ======================================================================
// \title  Stm32SpiDriverTestMain.cpp
// \author ivanlara
// \brief  cpp file for Stm32SpiDriver component test main function
// ======================================================================

#include "Stm32SpiDriverTester.hpp"

TEST(Nominal, OpenSuccess) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testOpenSuccess();
}

TEST(Nominal, OpenFailure) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testOpenFailure();
}

TEST(Nominal, OpenDefaultsToPolled) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testOpenDefaultsToPolled();
}

TEST(Nominal, OpenDma) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testOpenDma();
}

TEST(Nominal, SpiWriteReadDmaSuccess) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testSpiWriteReadDmaSuccess();
}

TEST(Nominal, SpiWriteReadDmaFailure) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testSpiWriteReadDmaFailure();
}

TEST(Nominal, SpiWriteReadDmaTimeout) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testSpiWriteReadDmaTimeout();
}

TEST(Nominal, ReopenSwitchesMode) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testReopenSwitchesMode();
}

TEST(Nominal, SpiWriteReadBeforeOpen) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testSpiWriteReadBeforeOpen();
}

TEST(Nominal, SpiReadWriteBeforeOpen) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testSpiReadWriteBeforeOpen();
}

TEST(Nominal, SpiWriteReadSuccess) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testSpiWriteReadSuccess();
}

TEST(Nominal, SpiWriteReadFailure) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testSpiWriteReadFailure();
}

TEST(Nominal, SpiReadWriteSuccess) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testSpiReadWriteSuccess();
}

TEST(Nominal, SpiWriteReadLargeBufferCapped) {
    Stm32::Stm32SpiDriverTester tester;
    tester.testSpiWriteReadLargeBufferCapped();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
