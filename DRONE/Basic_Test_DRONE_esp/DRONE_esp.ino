// created by pradeep using AI just to test the drone componets and connection no control is involved

#include <Arduino.h>
#include <Wire.h>
#include <RF24.h>
#include <SPI.h>

struct Ctrl {
  uint16_t throttle;
  int16_t  roll;
  int16_t  pitch;
  int16_t  yaw;
  uint8_t  arming;
} __attribute__((packed));

RF24 radio(4, 5);
const byte pipeAddr[6] = "DRONE";

Ctrl ctrl = {0,0,0,0,0};
unsigned long lastPkt = 0;

// ESC pins
const int M1_PIN = 13, M2_PIN = 12, M3_PIN = 14, M4_PIN = 27;
const int PWM_FREQ = 50;      // 50 Hz for standard RC
const int PWM_RES = 16;       // 16-bit resolution
const int MIN_US = 1000;
const int MAX_US = 2000;

void escAttach() {
  // In ESP32 Core 3.0+, we attach directly to the pin. 
  // ledcAttach(pin, freq, resolution)
  ledcAttach(M1_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(M2_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(M3_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(M4_PIN, PWM_FREQ, PWM_RES);
}

uint32_t usToDuty(int us) {
  const float period_us = 1000000.0f / PWM_FREQ; // 20000 us
  float dutyf = (float)us / period_us * ((1 << PWM_RES) - 1);
  return (uint32_t)constrain(dutyf, 0, (1 << PWM_RES) - 1);
}

void escWriteUS(int pin, int us) {
  us = constrain(us, MIN_US, MAX_US);
  // In Core 3.0+, we use the pin number here, not a channel
  ledcWrite(pin, usToDuty(us));
}

// Basic IMU (MPU6050) handling
float angleRoll=0, anglePitch=0;
float gyroBiasX=0, gyroBiasY=0, gyroBiasZ=0;
unsigned long lastIMUms=0;

void mpuInit() {
  Wire.begin(21, 22);
  delay(50);
  Wire.beginTransmission(0x68);
  Wire.write(0x6B); Wire.write(0x00); // Wake up
  Wire.endTransmission();
  
  Wire.beginTransmission(0x68); Wire.write(0x1B); Wire.write(0x00); Wire.endTransmission();
  Wire.beginTransmission(0x68); Wire.write(0x1C); Wire.write(0x00); Wire.endTransmission();
  delay(100);

  long gx=0, gy=0, gz=0;
  for(int i=0; i<500; i++){
    int16_t gxRaw, gyRaw, gzRaw;
    Wire.beginTransmission(0x68); Wire.write(0x43); Wire.endTransmission(false);
    Wire.requestFrom(0x68, 6, true);
    gxRaw=(Wire.read()<<8)|Wire.read(); gyRaw=(Wire.read()<<8)|Wire.read(); gzRaw=(Wire.read()<<8)|Wire.read();
    gx += gxRaw; gy += gyRaw; gz += gzRaw;
    delay(2);
  }
  gyroBiasX = gx/500.0f; gyroBiasY = gy/500.0f; gyroBiasZ = gz/500.0f;
  lastIMUms = millis();
}

void mpuUpdate() {
  int16_t ax, ay, az, gxRaw, gyRaw, gzRaw;
  Wire.beginTransmission(0x68); Wire.write(0x3B); Wire.endTransmission(false);
  Wire.requestFrom(0x68, 14, true);
  ax=(Wire.read()<<8)|Wire.read(); ay=(Wire.read()<<8)|Wire.read(); az=(Wire.read()<<8)|Wire.read();
  Wire.read(); Wire.read(); // Skip temp
  gxRaw=(Wire.read()<<8)|Wire.read(); gyRaw=(Wire.read()<<8)|Wire.read(); gzRaw=(Wire.read()<<8)|Wire.read();

  float axg = ax/16384.0f, ayg = ay/16384.0f, azg = az/16384.0f;
  float gxDps = (gxRaw - gyroBiasX)/131.0f;
  float gyDps = (gyRaw - gyroBiasY)/131.0f;

  unsigned long now = millis();
  float dt = (now - lastIMUms) / 1000.0f;
  lastIMUms = now;

  float rollAcc  = atan2f(ayg, azg) * 57.2958f;
  float pitchAcc = atan2f(-axg, sqrtf(ayg*ayg + azg*azg)) * 57.2958f;

  const float alpha = 0.98f;
  angleRoll  = alpha*(angleRoll  + gxDps*dt) + (1-alpha)*rollAcc;
  anglePitch = alpha*(anglePitch + gyDps*dt) + (1-alpha)*pitchAcc;
}

// Simple PID
float kp=1.2f, kd=0.05f, ki=0.0f;
float iRoll=0, iPitch=0;
float lastErrRoll=0, lastErrPitch=0;

void computeStabilization(float &mixRoll, float &mixPitch) {
  float targetRoll  = ctrl.roll * 0.04f;
  float targetPitch = ctrl.pitch * 0.04f;
  float errRoll  = targetRoll - angleRoll;
  float errPitch = targetPitch - anglePitch;

  iRoll += errRoll; iPitch += errPitch;
  float dRoll  = errRoll - lastErrRoll;
  float dPitch = errPitch - lastErrPitch;
  lastErrRoll = errRoll; lastErrPitch = errPitch;

  mixRoll  = kp*errRoll  + kd*dRoll  + ki*iRoll;
  mixPitch = kp*errPitch + kd*dPitch + ki*iPitch;
}

void setAllMotorsUS(int m1, int m2, int m3, int m4) {
  escWriteUS(M1_PIN, m1);
  escWriteUS(M2_PIN, m2);
  escWriteUS(M3_PIN, m3);
  escWriteUS(M4_PIN, m4);
}

void motorStop() { setAllMotorsUS(MIN_US, MIN_US, MIN_US, MIN_US); }

void setup() {
  Serial.begin(115200);
  escAttach();
  motorStop();
  mpuInit();

  if (!radio.begin()) Serial.println("RF init fail");
  radio.setAutoAck(true);
  radio.setRetries(2, 15);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(76);
  radio.openReadingPipe(0, pipeAddr);
  radio.startListening();

  unsigned long t0 = millis();
  while (millis() - t0 < 2000) {
    motorStop();
    delay(20);
  }
}

void loop() {
  if (radio.available()) {
    radio.read(&ctrl, sizeof(ctrl));
    lastPkt = millis();
  }

  bool linkActive = (millis() - lastPkt) < 200;
  if (!linkActive || ctrl.arming == 0) {
    motorStop();
    mpuUpdate();
    return;
  }

  mpuUpdate();
  float mixRoll=0, mixPitch=0;
  computeStabilization(mixRoll, mixPitch);

  int baseUs = map(ctrl.throttle, 0, 1000, 1100, 1900);
  int yawAdj = map(ctrl.yaw, -500, 500, -100, 100);

  int m1 = baseUs + (int)(+mixPitch + -mixRoll) + yawAdj;
  int m2 = baseUs + (int)(+mixPitch + +mixRoll) + -yawAdj;
  int m3 = baseUs + (int)(-mixPitch + -mixRoll) + -yawAdj;
  int m4 = baseUs + (int)(-mixPitch + +mixRoll) + yawAdj;

  setAllMotorsUS(m1, m2, m3, m4);
}
