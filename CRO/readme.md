# ESP32 CRO Web Server

A simple **oscilloscope (CRO)** implementation using an ESP32 and a web interface.  
This project allows you to view live waveforms from an analog input pin directly in your browser, with adjustable controls for time base, voltage scale, trigger level, and pause/resume.

---

## 🚀 Features
- Live waveform viewer on a web page (served by ESP32).
- Adjustable controls:
  - **Time Base** (sample delay, like time/div).
  - **Voltage Scale** (volts/div, display only).
  - **Trigger Level** (visual line + rising-edge trigger).
  - **Pause/Resume** waveform capture.
  
- Real-time statistics:
  - Vpp, Vmax, Vmin, Vavg, Vrms
  - Amplitude, Frequency, Period
- Trigger line overlay for stable waveform viewing.
- Responsive controls with sliders and buttons.

---

## 🛠 Hardware Requirements
- **ESP32 board** (with WiFi capability).
- Signal input connected to **GPIO34** (ADC pin).
- Voltage range: **0–3.3V only** (do not exceed).
- USB cable for programming and power.

---

## 📦 Software Requirements
- Arduino IDE (with ESP32 board support installed).
- Libraries:
  - `WiFi.h`
  - `WebServer.h`

---

## ⚙️ Setup Instructions
1. Clone or copy this project into your Arduino IDE.
2. Update WiFi credentials in the code:
   ```cpp
   const char* ssid = "YOUR_WIFI_NAME";
   const char* password = "YOUR_WIFI_PASSWORD";
