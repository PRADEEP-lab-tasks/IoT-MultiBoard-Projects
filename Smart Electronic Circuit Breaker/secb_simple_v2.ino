/*
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

// ---------- User-adjustable settings ----------
const float ACS712_SENS_V_PER_A  = 0.185;  // 5A module = 0.185 V/A, 20A module = 0.100 V/A
const float DIVIDER_RATIO        = 1.5;    // (R1+R2)/R2 for a 10k/20k divider on ACS712 OUT
const float TRIP_CURRENT_A       = 1.5;    // set above your rated load current
const unsigned long RECLOSE_WAIT_MS  = 3000;  // wait before each reclose attempt
const unsigned long RECLOSE_CHECK_MS = 100;   // watch window right after reclosing
const int MAX_RECLOSE_ATTEMPTS       = 3;
const unsigned long STARTUP_BLANK_MS = 300;   // ignore current right after turn-on
                                               // (rides out motor inrush - set to 0
                                               // for a purely resistive load)
const bool DEBUG_PRINTS = true;  // set false once everything reads correctly

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

// ---------- Pins ----------
const int MOSFET_GATE_PIN = 4;
const int ACS712_PIN      = 34;
const int BUZZER_PIN      = 5;
const int RESET_BTN_PIN   = 18;
const int STATUS_LED_PIN  = 2;

enum State { NORMAL, TRIPPED, RECLOSING, LOCKOUT };
State state = NORMAL;
unsigned long stateTimer = 0;
unsigned long motorOnTime = 0;
int failCount = 0;
float zeroOffsetV = 0.0;

void setup() {
  pinMode(MOSFET_GATE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);
  analogReadResolution(12);

  Serial.begin(115200);
  delay(200);

  digitalWrite(MOSFET_GATE_PIN, LOW);   // start OFF while we calibrate
  calibrateZero();
  digitalWrite(MOSFET_GATE_PIN, HIGH);  // switch ON, enter NORMAL
  motorOnTime = millis();
  Serial.println("SECB ready. State: NORMAL");
}

void loop() {
  float current = readCurrentAmps();

  static unsigned long lastDebug = 0;
  if (DEBUG_PRINTS && millis() - lastDebug > 250) {
    lastDebug = millis();
    int rawADC = analogRead(ACS712_PIN);
    Serial.printf("raw=%d  current=%.2fA  threshold=%.2fA  state=%d\n",
                  rawADC, current, TRIP_CURRENT_A, state);
  }

  switch (state) {

    case NORMAL:
      if (millis() - motorOnTime < STARTUP_BLANK_MS) break;  // let inrush settle
      if (current > TRIP_CURRENT_A) {
        // confirm with one more fast reading before tripping - cuts false
        // trips from ADC/sensor noise while adding well under 1ms of delay
        if (readCurrentAmps() > TRIP_CURRENT_A) {
          digitalWrite(MOSFET_GATE_PIN, LOW);   // <-- the actual trip
          tone(BUZZER_PIN, 2500, 150);
          Serial.printf("FAULT: %.2f A > %.2f A threshold. Tripped.\n", current, TRIP_CURRENT_A);
          state = TRIPPED;
          stateTimer = millis();
        }
      }
      break;

    case TRIPPED:
      digitalWrite(STATUS_LED_PIN, (millis() / 200) % 2);  // blink while tripped
      if (millis() - stateTimer >= RECLOSE_WAIT_MS) {
        state = RECLOSING;
        stateTimer = millis();
      }
      break;

    case RECLOSING: {
      Serial.println("Attempting reclose...");
      digitalWrite(MOSFET_GATE_PIN, HIGH);
      motorOnTime = millis();
      delay(STARTUP_BLANK_MS);  // let inrush settle before judging the reclose
                                 // (the flyback diode protects the FET meanwhile)

      unsigned long checkStart = millis();
      bool stillFaulted = false;
      while (millis() - checkStart < RECLOSE_CHECK_MS) {
        if (readCurrentAmps() > TRIP_CURRENT_A) { stillFaulted = true; break; }
      }

      if (stillFaulted) {
        digitalWrite(MOSFET_GATE_PIN, LOW);
        failCount++;
        Serial.printf("Reclose attempt %d failed - fault still present.\n", failCount);
        if (failCount >= MAX_RECLOSE_ATTEMPTS) {
          state = LOCKOUT;
          Serial.println("Max reclose attempts reached. LOCKOUT - press reset.");
        } else {
          state = TRIPPED;
          stateTimer = millis();
        }
      } else {
        Serial.println("Reclose successful. State: NORMAL");
        failCount = 0;
        state = NORMAL;
      }
      break;
    }

    case LOCKOUT:
      digitalWrite(MOSFET_GATE_PIN, LOW);
      digitalWrite(STATUS_LED_PIN, HIGH);
      if (digitalRead(RESET_BTN_PIN) == LOW) {
        delay(30);  // debounce - only used here, never in the trip path
        if (digitalRead(RESET_BTN_PIN) == LOW) {
          Serial.println("Manual reset pressed. Retrying...");
          failCount = 0;
          digitalWrite(STATUS_LED_PIN, LOW);
          state = RECLOSING;
          stateTimer = millis();
        }
      }
      break;
  }
}

float readCurrentAmps() {
  int raw = analogRead(ACS712_PIN);
  float vAtPin = raw * (3.3 / 4095.0);
  float vAtSensorOut = vAtPin * DIVIDER_RATIO;      // undo the voltage divider
  return (vAtSensorOut - zeroOffsetV) / ACS712_SENS_V_PER_A;
}

void calibrateZero() {
  // MOSFET is OFF here, so no current is flowing - average many readings
  // to find the sensor's true zero-current output voltage.
  long sum = 0;
  const int N = 200;
  for (int i = 0; i < N; i++) {
    sum += analogRead(ACS712_PIN);
    delay(2);
  }
  float avgRaw = sum / (float)N;
  float vAtPin = avgRaw * (3.3 / 4095.0);
  zeroOffsetV = vAtPin * DIVIDER_RATIO;
  Serial.printf("Zero-current calibration: %.3f V\n", zeroOffsetV);
}
