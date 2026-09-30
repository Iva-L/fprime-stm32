// ======================================================================
// \title  Stm32I2cDriverTestMain.cpp
// \author ivanlara
// \brief  cpp file for Stm32I2cDriver component test main function
// ======================================================================

#include "Stm32I2cDriverTester.hpp"

TEST(Nominal, OpenSuccessFast) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testOpenSuccessFast();
}

TEST(Nominal, OpenSuccessStandard) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testOpenSuccessStandard();
}

TEST(Nominal, OpenSuccessFastPlus) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testOpenSuccessFastPlus();
}

TEST(Nominal, OpenFailure) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testOpenFailure();
}

TEST(Nominal, WriteBeforeOpen) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testWriteBeforeOpen();
}

TEST(Nominal, ReadBeforeOpen) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testReadBeforeOpen();
}

TEST(Nominal, WriteReadBeforeOpen) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testWriteReadBeforeOpen();
}

TEST(Nominal, WriteSuccess) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testWriteSuccess();
}

TEST(Nominal, WriteFailure) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testWriteFailure();
}

TEST(Nominal, WriteLargeBufferCapped) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testWriteLargeBufferCapped();
}

TEST(Nominal, ReadSuccess) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testReadSuccess();
}

TEST(Nominal, ReadFailure) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testReadFailure();
}

TEST(Nominal, ReadLargeBufferCapped) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testReadLargeBufferCapped();
}

TEST(Nominal, WriteReadSuccess) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testWriteReadSuccess();
}

TEST(Nominal, WriteReadTransmitFailureShortCircuits) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testWriteReadTransmitFailureShortCircuits();
}

TEST(Nominal, WriteReadReceiveFailure) {
    Stm32::Stm32I2cDriverTester tester;
    tester.testWriteReadReceiveFailure();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
