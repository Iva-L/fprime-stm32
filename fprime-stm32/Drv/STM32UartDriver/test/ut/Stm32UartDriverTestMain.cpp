// ======================================================================
// \title  Stm32UartDriverTestMain.cpp
// \author ivanlara
// \brief  cpp file for Stm32UartDriver component test main function
// ======================================================================

#include "Stm32UartDriverTester.hpp"

TEST(Nominal, OpenSuccess) {
    Stm32::Stm32UartDriverTester tester;
    tester.testOpenSuccess();
}

TEST(Nominal, OpenFailure) {
    Stm32::Stm32UartDriverTester tester;
    tester.testOpenFailure();
}

TEST(Nominal, SendInvalidBuffer) {
    Stm32::Stm32UartDriverTester tester;
    tester.testSendInvalidBuffer();
}

TEST(Nominal, SendFits) {
    Stm32::Stm32UartDriverTester tester;
    tester.testSendFits();
}

TEST(Nominal, SendRejectedWhenFull) {
    Stm32::Stm32UartDriverTester tester;
    tester.testSendRejectedWhenFull();
}

TEST(Nominal, PollDrainsTx) {
    Stm32::Stm32UartDriverTester tester;
    tester.testPollDrainsTx();
}

TEST(Nominal, PollTxHwStartFailure) {
    Stm32::Stm32UartDriverTester tester;
    tester.testPollTxHwStartFailure();
}

TEST(Nominal, PollTxBusyThenComplete) {
    Stm32::Stm32UartDriverTester tester;
    tester.testPollTxBusyThenComplete();
}

TEST(Nominal, TxWatchdogTimeout) {
    Stm32::Stm32UartDriverTester tester;
    tester.testTxWatchdogTimeout();
}

TEST(Nominal, RecvReturnIn) {
    Stm32::Stm32UartDriverTester tester;
    tester.testRecvReturnIn();
}

TEST(Nominal, PollDrainsRx) {
    Stm32::Stm32UartDriverTester tester;
    tester.testPollDrainsRx();
}

TEST(Nominal, PollRxNoBuffers) {
    Stm32::Stm32UartDriverTester tester;
    tester.testPollRxNoBuffers();
}

TEST(Nominal, PollRxRingFull) {
    Stm32::Stm32UartDriverTester tester;
    tester.testPollRxRingFull();
}

TEST(Nominal, UartErrorRecoveryRxOnly) {
    Stm32::Stm32UartDriverTester tester;
    tester.testUartErrorRecoveryRxOnly();
}

TEST(Nominal, UartErrorRecoveryDmaError) {
    Stm32::Stm32UartDriverTester tester;
    tester.testUartErrorRecoveryDmaError();
}

TEST(Nominal, TwoInstancesDoNotInterfere) {
    Stm32::Stm32UartDriverTester tester;
    tester.testTwoInstancesDoNotInterfere();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
