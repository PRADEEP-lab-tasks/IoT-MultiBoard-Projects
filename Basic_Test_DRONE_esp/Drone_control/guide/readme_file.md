# 🛠️ Step‑by‑Step Guide

## 🔧 Hardware Setup
- ESP32 development board
- nRF24L01 RF module (with capacitor across VCC–GND for stability)
- Connect ESP32 via USB to your computer
- If controlling motors/ESCs for a drone, wire them to the correct GPIO pins (with external power supply for motors)

### Wiring (ESP32 → nRF24L01)
- CE → GPIO4  
- CSN → GPIO5  
- MOSI → GPIO23 (default SPI)  
- MISO → GPIO19 (default SPI)  
- SCK → GPIO18 (default SPI)  
- VCC → 3.3V (stable supply)  
- GND → GND  

---

## 💻 Install Arduino IDE
- Download and install [Arduino IDE](https://www.arduino.cc/en/software).
- In **Preferences**, add ESP32 board manager URL:  






--------------------------------------------------
SOFTWARE SETUP


- Go to **Tools → Board → Boards Manager** and install **ESP32**.

---

## 📚 Add Libraries
- In Arduino IDE, go to **Sketch → Include Library → Manage Libraries**.
- Install **RF24** library.
- Ensure **WiFi** and **WebServer** are available (they come with ESP32 core).

---

## ⚙️ Configure Your Code
- Upload the provided code to your ESP32.
- The ESP32 will create a Wi‑Fi Access Point:
- SSID: `DroneGround`
- Password: `drone1234`

---

## 🌐 Set Up Web Controller
- Connect your phone/laptop to the `DroneGround` Wi‑Fi network.
- Open browser → go to `192.168.4.1`.
- You’ll see the **Drone Controller webpage** with sliders and buttons:
- Throttle (0–1000)
- Roll (−500 to +500)
- Pitch (−500 to +500)
- Yaw (−500 to +500)
- Arm / Disarm buttons

---

## 📡 Run and Monitor
- Open **Serial Monitor** in Arduino IDE (baud rate 115200).
- Move sliders/joysticks in the web app.
- You’ll see live values printed:
