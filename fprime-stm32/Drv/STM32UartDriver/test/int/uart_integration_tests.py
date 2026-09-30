"""! @file uart_integration_tests.py
@brief Integration tests for the UART driver component.
@details This file uses the F' test API to send commands to the UART component and assert the expected responses.
@note The script can be executed with `pytest lib/fprime-stm32/Drv/STM32UartDriver/test/int/uart_integration_tests.py --dictionary build-artifacts/stm32h7/ReferenceDeployment/dict/ReferenceDeploymentTopologyDictionary.json`
"""

import time
from fprime_gds.common.testing_fw import predicates

def test_cmd_no_op(fprime_test_api):
    """
    @brief Test that CMD_NO_OP can be sent and return without any errors proving the UART driver is responsive
    @param fprime_test_api: The test API instance used to send commands and assert responses
    @return: None
    """
    fprime_test_api.send_and_assert_command("CdhCore.cmdDisp.CMD_NO_OP")