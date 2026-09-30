module Stm32 {
    passive component STM32Timer {

        ###############################################################################
        # Drv.Tick: output port raising a cycle tick, meant to be connected to
        # Svc.RateGroupDriver.CycleIn -- the same contract the prior Stm32.Timer
        # stub exposed, so it drops into the existing topology wiring unchanged.
        ###############################################################################
        import Drv.Tick

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel

        ###############################################################################
        # Events
        ###############################################################################

        @ TIM2 output-compare tick source configured and armed
        event Configured(
                             periodUs: U32 @< Configured tick period in microseconds
                           ) \
          severity activity high \
          id 0 \
          format "TIM2 tick source armed for a {} us period"

        @ Tick overrun event: indicates that the TIM2 compare target was missed
        event TickOverrun(
                              lateUs: U32 @< Microseconds the compare target had already elapsed by
                            ) \
          severity warning high \
          id 1 \
          format "TIM2 tick overran by {} us; compare resynced to now + period" \
          throttle 20

        ###############################################################################
        # Telemetry
        ###############################################################################

        @ Total number of CycleOut ticks raised since the timer was armed
        telemetry TickCount: U32 id 0

        @ Total number of ticks that overran (see TickOverrun)
        telemetry OverrunCount: U32 id 1

    }
}