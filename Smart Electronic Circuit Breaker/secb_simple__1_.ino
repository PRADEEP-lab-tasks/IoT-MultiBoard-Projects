// Created by Pradeep with the help of AI

// ---------- User-adjustable settings ----------
const float ACS712_SENS_V_PER_A  = 0.185;  // 5A module = 0.185 V/A, 20A module = 0.100 V/A
const float DIVIDER_RATIO        = 1.5;    // (R1+R2)/R2 for a 10k/20k divider on ACS712 OUT
const float TRIP_CURRENT_A       = 1;    // set above your rated load current
const unsigned long RECLOSE_WAIT_MS  = 3000;  // wait before each reclose attempt
const unsigned long RECLOSE_CHECK_MS = 100;   // watch window right after reclosing
const int MAX_RECLOSE_ATTEMPTS       = 3;
const unsigned long STARTUP_BLANK_MS = 300;   // ignore current right after turn-on
                                               // (rides out motor inrush - set to 0
                                               // for a purely resistive load)

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
