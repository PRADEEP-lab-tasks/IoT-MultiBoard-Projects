Simplified Smart Electronic Circuit Breaker (SECB)
  Single-MCU version: the ESP32 does both fault detection and control.
  No comparator/latch/AND-gate hardware - just current sensing, a MOSFET
  switch, and a state machine for trip / auto-reclose / lockout.

  Wiring summary:
    PSU+  -> Load (power resistor) -> ACS712 IP+
    ACS712 IP- -> MOSFET Drain
    MOSFET Source -> GND (common with ESP32 GND and PSU-)
    MOSFET Gate  <- 220R <- ESP32 GPIO4      (gate drive)
    MOSFET Gate  -> 10k  -> GND               (pulldown, keeps FET off by default)
    ACS712 OUT   -> 10k/20k divider -> ESP32 GPIO34 (ADC, keeps signal under 3.3V)
    ACS712 VCC -> 5V, ACS712 GND -> common GND
    Buzzer  <- ESP32 GPIO5
    Reset button -> ESP32 GPIO18 (INPUT_PULLUP, active LOW)
    Status LED   <- ESP32 GPIO2 (optional)

  If your load is a DC motor instead of a resistor:
    - Add a flyback diode straight across the motor terminals - cathode
      to the PSU+ side, anode to the ACS712/drain side. Protects the
      MOSFET from the inductive kick when it switches off.
    - STARTUP_BLANK_MS below rides out motor inrush current so a normal
      start isn't mistaken for a fault. Tune it to your motor.

  No extra libraries needed. Board: your ESP32 dev board (esp32 by
  Espressif Systems in Boards Manager).
*/



/*
  CALIBRATING ACS712_SENS_V_PER_A TO YOUR ACTUAL BOARD
  (do this if the breaker isn't tripping even though you know the real
  current is well above TRIP_CURRENT_A):

  1. Leave DEBUG_PRINTS = true. Temporarily set TRIP_CURRENT_A to something
     high like 20 so nothing trips while you're just measuring.
  2. Run the motor alone. Watch the Serial Monitor "current" value settle -
     it should read close to 0.5A (or whatever your motor really draws).
  3. Add the fault bulb in parallel. Watch "current" again - it should
     jump to close to the real combined value (~3A).
  4. If the printed values barely move at all between steps 2 and 3, the
     sensor isn't seeing the current - that's a wiring problem (see IP+/IP-
     note above), not a math problem. Fix the wiring first.
  5. If the printed values DO move but land far from the real numbers by a
     consistent ratio, your sensitivity constant is wrong for your board.
     Correct it:  new_SENS = old_SENS * (printed_current / real_current)
     Example: printed 1.6A when it's really 3A -> ratio 0.53
              0.185 * 0.53 = 0.098  -> you likely have a 20A module.
  6. Once printed current tracks real current, set TRIP_CURRENT_A back to
     something between your motor's running current and the fault current
     (e.g. 1.2-1.5A here), and set DEBUG_PRINTS = false.
*/
