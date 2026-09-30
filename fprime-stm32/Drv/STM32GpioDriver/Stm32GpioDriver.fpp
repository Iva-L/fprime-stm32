module Stm32 {

  passive component Stm32GpioDriver {

    import Drv.Gpio

    event port Log

    text event port LogText

    time get port Time

    @ Emitted when the GPIO pin fails to configure for the requested mode.
    event ConfigureError(
                             pin: U32 @< The pin bitmask passed to open()
                             requestedMode: Fw.Direction @< The mode that was requested
                           ) \
      severity warning high \
      id 0 \
      format "GPIO pin 0x{x} failed to configure for requested mode {}"

    @ Emitted when the GPIO pin is successfully configured for the requested mode.
    event ConfigureSuccess(
                             pin: U32 @< The pin bitmask passed to open()
                             requestedMode: Fw.Direction @< The mode that was requested
                           ) \
      severity activity high \
      id 1 \
      format "GPIO pin 0x{x} successfully configured for requested mode {}"

  }

}
