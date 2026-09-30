
####
# STM32H7 bare-metal OSAL implementations. OSAL backends are family-specific
# and mutually exclusive, so this subdirectory is only ever relevant on the
# stm32h7 platform.
####
restrict_platforms(stm32h7)

register_os_implementation(Task Stm32H7 FprimeStm32 fprime-baremetal_Os_TaskRunner)
register_os_implementation(Mutex Stm32H7 FprimeStm32)
register_os_implementation(RawTime Stm32H7 FprimeStm32)
register_os_implementation(Queue Stm32H7 FprimeStm32)
