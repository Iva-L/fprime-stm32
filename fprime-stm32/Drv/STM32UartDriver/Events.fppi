@ USART1 configured and DMA reception armed
event PortOpened(
                     baud: U32 @< The configured baud rate
                   ) \
  severity activity high \
  id 0 \
  format "USART1 configured at {} baud with DMA TX/RX"

@ A HAL call returned a non-OK status
event HalError(
                   op: string size 24 @< The HAL operation that failed
                   status: I32 @< The HAL_StatusTypeDef value
                 ) \
  severity warning high \
  id 1 \
  format "USART1 HAL operation {} failed: {}" \
  throttle 1 every { seconds = 10 }

@ USART1 reported a line/overrun/DMA error via huart1.ErrorCode
event UartError(
                    errorCode: U32 @< The latched HAL UART ErrorCode bitmask
                  ) \
  severity warning high \
  id 2 \
  format "USART1 error, HAL ErrorCode 0x{x}" \
  throttle 1 every { seconds = 10 }

@ The internal TX ring buffer had no room for a send request
event TxRingFull(
                     requested: U32 @< Bytes requested to enqueue
                     available: U32 @< Bytes actually free in the ring
                   ) \
  severity warning high \
  id 3 \
  format "USART1 TX ring full, requested {} bytes, {} available" \
  throttle 1 every { seconds = 10 }

@ The internal RX ring buffer had no room for newly received data
event RxRingFull(
                     dropped: U32 @< Bytes dropped from the RX ring
                   ) \
  severity warning high \
  id 4 \
  format "USART1 RX ring full, dropped {} bytes" \
  throttle 1 every { seconds = 10 }

@ A TX DMA transfer did not complete within the transaction watchdog
event TxTimeout(
                    elapsedUs: U32 @< Measured elapsed time in microseconds
                  ) \
  severity warning high \
  id 5 \
  format "USART1 TX DMA timed out after {} us" \
  throttle 1 every { seconds = 10 }

@ USART1 ran out of Fw::Buffer allocations for an incoming frame
event NoBuffers() \
  severity warning high \
  id 6 \
  format "USART1 ran out of buffers for received data" \
  throttle 1 every { seconds = 10 }
