// ======================================================================
// \title  STM32TimerTestMain.cpp
// \author ivanlara
// \brief  cpp file for STM32Timer component test main function
// ======================================================================

#include "STM32TimerTester.hpp"

TEST(Nominal, OpenArms) {
    Stm32::STM32TimerTester tester;
    tester.testOpenArms();
}

TEST(Nominal, PollWithoutTick) {
    Stm32::STM32TimerTester tester;
    tester.testPollWithoutTick();
}

TEST(Nominal, PollWithTick) {
    Stm32::STM32TimerTester tester;
    tester.testPollWithTick();
}

TEST(Nominal, OverrunDetection) {
    Stm32::STM32TimerTester tester;
    tester.testOverrunDetection();
}

TEST(Nominal, PollWithTickNoOverrunAtExactBoundary) {
    Stm32::STM32TimerTester tester;
    tester.testPollWithTickNoOverrunAtExactBoundary();
}

TEST(Nominal, TwoInstancesDoNotInterfere) {
    Stm32::STM32TimerTester tester;
    tester.testTwoInstancesDoNotInterfere();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
