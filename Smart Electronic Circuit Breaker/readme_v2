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
