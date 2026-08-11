# 🚁 IoT Drone Control with ESP32-CAM & MPU6050

This project demonstrates a **DIY drone control system** using an **ESP32-CAM board**, **MPU6050 IMU sensor**, and **Electronic Speed Controllers (ESCs)** for brushless motors.  
It integrates **camera streaming**, **Wi-Fi control**, and **basic stabilization logic** using sensor fusion.

---

## 📌 Features
- Live video streaming via ESP32-CAM (`http://192.168.4.1/stream`).
- SoftAP Wi-Fi access point for direct connection (no router needed).
- ESC motor control using PWM signals.
- Basic stabilization using MPU6050 gyroscope + accelerometer.
- Web-based control endpoint (`/control?cmd=up` or `/control?cmd=down`).
- Two versions of code:
  - **ESP32-CAM + MPU6050 + WebServer** (Wi-Fi + camera streaming).
  - **Arduino + MPU6050 + ESC** (basic stabilization and throttle control).

---

## 🛠️ Hardware Requirements
- ESP32-CAM (AI Thinker module)
- MPU6050 (Gyroscope + Accelerometer)
- 4 × Brushless motors + ESCs
- Power supply (LiPo battery recommended)
- Arduino Uno/Nano (for standalone ESC + MPU6050 testing)
- Jumper wires, breadboard

---

## 📂 Code Overview

### ESP32-CAM Drone Control
- Configures ESP32-CAM pins for camera streaming.
- Sets up **PWM channels** for 4 motors.
- Runs a **WebServer** with `/control` endpoint:
  - `cmd=up` → increases throttle.
  - `cmd=down` → decreases throttle.
- Uses **complementary filter** for angle estimation:
  ```cpp
  angleX = 0.98 * (angleX + mpu.getGyroX() * dt) + 0.02 * mpu.getAccX();
  angleY = 0.98 * (angleY + mpu.getGyroY() * dt) + 0.02 * mpu.getAccY();


  | Component | Pin |
| --- | --- |
| Motor 1 ESC | GPIO12 |
| Motor 2 ESC | GPIO13 |
| Motor 3 ESC | GPIO14 |
| Motor 4 ESC | GPIO15 |
| MPU6050 SDA | GPIO26 |
| MPU6050 SCL | GPIO27 |
| Camera pins | Default AI Thinker config |


| Component | Pin |
| --- | --- |
| ESC Signal | D9 |
| MPU6050 SDA | A4 |
| MPU6050 SCL | A5 |
