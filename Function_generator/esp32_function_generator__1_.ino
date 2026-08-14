//created by pradeep

/*
  ESP32 Function Generator (DAC-based)
  -------------------------------------
  Generates Sine / Triangle / Sawtooth / Square waves on GPIO25 (DAC1).
  Output range: 0 - 3.3V (8-bit DAC, 0-255).

  Good for testing your ESP32 CRO project - connect DAC output (GPIO25)
  through a small series resistor to the ADC input (GPIO34) of the
  oscilloscope ESP32, or straight to a real scope.

  Control via Serial Monitor commands:
    s <freq>   -> sine wave, e.g. "s 1000" for 1kHz
    t <freq>   -> triangle wave
    w <freq>   -> sawtooth wave
    q <freq>   -> square wave
    a <0-255>  -> set amplitude (max DAC value used)
*/

#include "driver/dac.h"
#include "driver/timer.h"

#define DAC_PIN 25            // DAC1 = GPIO25, DAC2 = GPIO26
#define TABLE_SIZE 256

uint8_t sineTable[TABLE_SIZE];
uint8_t triangleTable[TABLE_SIZE];
uint8_t sawtoothTable[TABLE_SIZE];

volatile uint8_t *activeTable = sineTable;
volatile uint16_t phaseIndex = 0;
volatile uint16_t phaseIncrement = 1;
volatile uint8_t amplitude = 255;
volatile bool squareMode = false;
volatile bool squareState = false;

hw_timer_t *timer = NULL;
double sampleRateHz = 40000;   // fixed sample rate for the DAC update ISR

void buildTables() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    float angle = (2.0 * PI * i) / TABLE_SIZE;
    sineTable[i] = (uint8_t)(127.5 + 127.5 * sin(angle));
    triangleTable[i] = (i < TABLE_SIZE / 2)
                          ? (uint8_t)(i * 2)
                          : (uint8_t)(255 - (i - TABLE_SIZE / 2) * 2);
    sawtoothTable[i] = (uint8_t)i;
  }
}

void IRAM_ATTR onTimer() {
  if (squareMode) {
    dac_output_voltage(DAC_CHANNEL_1, squareState ? amplitude : 0);
    phaseIndex += phaseIncrement;
    if (phaseIndex < phaseIncrement) squareState = !squareState; // toggled on wrap
  } else {
    uint8_t raw = activeTable[phaseIndex >> 8];   // use top 8 bits of 16-bit phase
    uint8_t scaled = ((uint16_t)raw * amplitude) / 255;
    dac_output_voltage(DAC_CHANNEL_1, scaled);
    phaseIndex += phaseIncrement;
  }
}

void setFrequency(double freqHz) {
  // phaseIncrement chosen so that phaseIndex (16-bit) wraps at desired freq
  // wraps per second = sampleRateHz / (65536 / phaseIncrement) = freqHz
  double inc = (freqHz * 65536.0) / sampleRateHz;
  phaseIncrement = (uint16_t)inc;
  if (phaseIncrement < 1) phaseIncrement = 1;
}

void setup() {
  Serial.begin(115200);
  buildTables();
  dac_output_enable(DAC_CHANNEL_1);

  timer = timerBegin(1000000);           // 80MHz / 80 = 1MHz timer clock
 timerAttachInterrupt(timer, &onTimer);
 timerAlarm(timer, 1000000 / sampleRateHz, true, 0); // interval in us, autoreload, repeat forever
 // timerAlarmEnable(timer);

  activeTable = sineTable;
  setFrequency(1000);

  Serial.println("ESP32 Function Generator ready.");
  Serial.println("Commands: s <freq> | t <freq> | w <freq> | q <freq> | a <0-255>");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    Serial.readStringUntil(' ');   // consume space if present via parseFloat below
    double val = Serial.parseFloat();

    switch (cmd) {
      case 's':
        squareMode = false;
        activeTable = sineTable;
        setFrequency(val);
        Serial.printf("Sine wave @ %.2f Hz\n", val);
        break;
      case 't':
        squareMode = false;
        activeTable = triangleTable;
        setFrequency(val);
        Serial.printf("Triangle wave @ %.2f Hz\n", val);
        break;
      case 'w':
        squareMode = false;
        activeTable = sawtoothTable;
        setFrequency(val);
        Serial.printf("Sawtooth wave @ %.2f Hz\n", val);
        break;
      case 'q':
        squareMode = true;
        setFrequency(val);
        Serial.printf("Square wave @ %.2f Hz\n", val);
        break;
      case 'a':
        amplitude = (uint8_t)constrain(val, 0, 255);
        Serial.printf("Amplitude set to %d\n", amplitude);
        break;
    }
  }
}
