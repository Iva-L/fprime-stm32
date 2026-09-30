@ The I2C bus was configured and is ready to accept transactions
event PortOpened( 
                     speed: string size 8 @< The configured I2C bus speed
                   ) \
  severity activity high \
  id 0 \
  format "I2C bus configured at speed {}"

@ A blocking I2C HAL call returned a non-OK status
event HalError(
                   op: string size 24 @< The HAL operation that failed
                   addr: U32 @< The 7-bit I2C slave address involved
                   status: I32 @< The HAL_StatusTypeDef value
                 ) \
  severity warning high \
  id 1 \
  format "I2C HAL operation {} to address 0x{x} failed: {}" \
  throttle 1 every { seconds = 10 }