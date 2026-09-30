// ======================================================================
// \title  Stm32GpioDriverTestMain.cpp
// \author ivanlara
// \brief  cpp file for Stm32GpioDriver component test main function
// ======================================================================

#include "Stm32GpioDriverTester.hpp"

TEST(Nominal, OpenOutputSuccess) {
    Stm32::Stm32GpioDriverTester tester;
    tester.testOpenOutputSuccess();
}

TEST(Nominal, OpenInputSuccess) {
    Stm32::Stm32GpioDriverTester tester;
    tester.testOpenInputSuccess();
}

TEST(Nominal, OpenFailure) {
    Stm32::Stm32GpioDriverTester tester;
    tester.testOpenFailure();
}

TEST(Nominal, AccessBeforeOpen) {
    Stm32::Stm32GpioDriverTester tester;
    tester.testAccessBeforeOpen();
}

TEST(Nominal, WriteWrongMode) {
    Stm32::Stm32GpioDriverTester tester;
    tester.testWriteWrongMode();
}

TEST(Nominal, ReadWrongMode) {
    Stm32::Stm32GpioDriverTester tester;
    tester.testReadWrongMode();
}

TEST(Nominal, WriteAfterOpen) {
    Stm32::Stm32GpioDriverTester tester;
    tester.testWriteAfterOpen();
}

TEST(Nominal, ReadAfterOpen) {
    Stm32::Stm32GpioDriverTester tester;
    tester.testReadAfterOpen();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
