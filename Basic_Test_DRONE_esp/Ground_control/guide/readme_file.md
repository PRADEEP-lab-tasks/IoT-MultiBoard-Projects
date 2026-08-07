# 🚀 Drone Ground Control (ESP32 + RF24)

This project implements a **Wi-Fi based ground control station** using an ESP32 and an **nRF24L01 radio module**.  
It hosts a web interface where you can control **Throttle, Roll, Pitch, Yaw, and Arming** in real time, and transmits these commands via RF24 to the drone.

---

## 📦 Hardware Requirements
- ESP32 development board
- nRF24L01 RF module (with capacitor across VCC–GND for stability)
- Drone flight controller or microcontroller with RF24 receiver
- Power supply for motors/ESCs (separate from ESP32)

---

## 🔌 Wiring
ESP32 → nRF24L01
- CE → GPIO4  
- CSN → GPIO5  
- MOSI → GPIO23 (default SPI)  
- MISO → GPIO19 (default SPI)  
- SCK → GPIO18 (default SPI)  
- VCC → 3.3V (stable supply)  
- GND → GND  

---

## 🛠️ Software Setup
1. Install **Arduino IDE** and ESP32 board support:
   - Board Manager URL: `https://dl.espressif.com/dl/package_esp32_index.json`
2. Install required libraries:
   - `RF24`
   - `WiFi` (comes with ESP32 core)
   - `WebServer` (comes with ESP32 core)
3. Upload the provided code to your ESP32.

---

## 🌐 Wi-Fi Access Point
- ESP32 creates a Wi-Fi AP:
  - SSID: `DroneGround`
  - Password: `drone1234`
- Connect your phone/laptop to this AP.
- Open browser → go to `192.168.4.1`.

---

## 🎮 Web Controller
The ESP32 serves a **web interface** with sliders and buttons:

- **Throttle**: 0–1000  
- **Roll**: -500 to +500  
- **Pitch**: -500 to +500  
- **Yaw**: -500 to +500  
- **Arm/Disarm** buttons  

Commands are sent every **50 ms (20 Hz)** to the ESP32, which transmits them via RF24.

---

## 📡 RF24 Transmission
- Data is packed into a `Ctrl` struct:
  ```cpp
  struct Ctrl {
    uint16_t throttle;
    int16_t  roll;
    int16_t  pitch;
    int16_t  yaw;
    uint8_t  arming;
  } __attribute__((packed));
