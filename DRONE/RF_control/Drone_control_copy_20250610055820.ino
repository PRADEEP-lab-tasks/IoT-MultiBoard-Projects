#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Arduino.h>
#include "MPU6050_light.h"

// WiFi AP credentials
const char* ssid = "REC";
const char* password = "88888888";

// Motor output pins
const int motorPins[4] = {12, 13, 14, 15};

// ESC pulse parameters
const int freq = 50;
const int pwmResolution = 16;
int channel[4];

// MPU6050 object
MPU6050 mpu(Wire);
float angleX = 0, angleY = 0;
unsigned long lastUpdate = 0;
float dt = 0;

// HTTP server
WebServer server(80);

// Camera config (AI-Thinker board default pins)
camera_config_t config = {
  .pin_pwdn       = 32,
  .pin_reset      = -1,
  .pin_xclk       = 0,
  .pin_sccb_sda   = 26,
  .pin_sccb_scl   = 27,
  .pin_d7         = 35,
  .pin_d6         = 34,
  .pin_d5         = 39,
  .pin_d4         = 36,
  .pin_d3         = 21,
  .pin_d2         = 19,
  .pin_d1         = 18,
  .pin_d0         = 5,
  .pin_vsync      = 25,
  .pin_href       = 23,
  .pin_pclk       = 22,
  .xclk_freq_hz   = 20000000,
  .pixel_format   = PIXFORMAT_JPEG,
  .frame_size     = FRAMESIZE_VGA,
  .jpeg_quality   = 12,
  .fb_count       = 1
};

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void handleControl() {
  String cmd = server.arg("cmd");
  int speed = 1300;

  if (cmd == "up") speed += 200;
  if (cmd == "down") speed -= 200;

  for (int i = 0; i < 4; i++) {
    ledcWrite(channel[i], map(speed, 1000, 2000, 0, (1 << pwmResolution) - 1));
  }

  server.send(200, "text/plain", "OK");
}

void startCameraServer();  // Declare the function if it's defined elsewhere

void setup() {
  Serial.begin(115200);

  Wire.begin(0, 2);
  mpu.begin();
  mpu.calcGyroOffsets();

  for (int i = 0; i < 4; i++) {
    channel[i] = i;
    ledcSetup(channel[i], freq, pwmResolution);
    ledcAttachPin(motorPins[i], channel[i]);
    ledcWrite(channel[i], 0);
  }

  WiFi.softAP(ssid, password);

  esp_camera_init(&config);
  startCameraServer();

  server.on("/control", HTTP_GET, handleControl);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("Setup complete. Connect to WiFi and open http://192.168.4.1/stream");
  lastUpdate = millis();
}

void loop() {
  server.handleClient();
  mpu.update();

  unsigned long currentTime = millis();
  dt = (currentTime - lastUpdate) / 1000.0;
  lastUpdate = currentTime;

  angleX = 0.98 * (angleX + mpu.getGyroX() * dt) + 0.02 * mpu.getAccX();
  angleY = 0.98 * (angleY + mpu.getGyroY() * dt) + 0.02 * mpu.getAccY();
}










#include <Wire.h>
#include <MPU6050.h>
MPU6050 mpu;

const int escPin = 9; // PWM pin to ESC
int throttle = 1000;  // Base throttle (1000–2000 µs)

void setup() {
  Serial.begin(9600);
  Wire.begin();
  mpu.initialize();
  pinMode(escPin, OUTPUT);
  // Arm ESC with minimum throttle
  for (int i = 0; i < 50; i++) {
    writeESC(throttle);
    delay(20);
  }
  Serial.println("ESC Armed. MPU6050 Initialized.");
}

void loop() {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Simple pitch/roll estimation (not accurate, just for testing)
  float pitch = atan2(ax, az) * 180 / PI;
  float roll  = atan2(ay, az) * 180 / PI;

  // Adjust throttle based on pitch/roll (very basic)
  int adjustedThrottle = throttle + (int)(pitch * 2.0) - (int)(roll * 2.0);
  adjustedThrottle = constrain(adjustedThrottle, 1000, 2000);

  writeESC(adjustedThrottle);

  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print(" Roll: "); Serial.print(roll);
  Serial.print(" Throttle: "); Serial.println(adjustedThrottle);

  delay(50);
}

void writeESC(int pulseWidth) {
  // Send PWM signal to ESC
  digitalWrite(escPin, HIGH);
  delayMicroseconds(pulseWidth);
  digitalWrite(escPin, LOW);
  delay(20 - pulseWidth / 1000); // Maintain 50Hz signal
}
