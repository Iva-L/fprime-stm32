@ The SPI bus was configured and is ready to accept transactions
event PortOpened(
                     busInstance: string size 8 @< The configured SPI peripheral instance
                   ) \
  severity activity high \
  id 0 \
  format "SPI bus {} configured"

@ A blocking SPI HAL call returned a non-OK status
event HalError(
                   op: string size 24 @< The HAL operation that failed
                   status: I32 @< The HAL_StatusTypeDef value
                 ) \
  severity warning high \
  id 1 \
  format "SPI HAL operation {} failed: {}" \
  throttle 1 every { seconds = 10 }
