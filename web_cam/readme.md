
# 📷 ESP32‑CAM MJPEG Streaming Server

This project sets up an **ESP32‑CAM (AI Thinker module)** to stream live video over Wi‑Fi using a simple HTTP server. The stream can be viewed in any modern web browser.

---

## ⚙️ Requirements

### Hardware
- ESP32‑CAM (AI Thinker recommended)
- FTDI programmer (USB‑to‑Serial adapter)
- Jumper wires
- Micro‑USB cable
- Stable 5V power supply (≥1A recommended)

### Software
- Arduino IDE (latest version)
- ESP32 board support (`https://dl.espressif.com/dl/package_esp32_index.json`)
- Libraries: `esp_camera.h`, `WiFi.h`, `WebServer.h` (included in ESP32 core)

---

## 🔌 Wiring for Flashing

Connect ESP32‑CAM to FTDI programmer:

| ESP32‑CAM Pin | FTDI Pin |
|---------------|----------|
| U0R           | TX       |
| U0T           | RX       |
| GND           | GND      |
| 5V            | 5V       |

- **IO0 → GND** (to enable flashing mode)

---

## 🚀 Flashing & Setup

1. Open Arduino IDE.
2. Select **Board: ESP32 Wrover Module**.
3. Set **Partition Scheme: Huge APP (3MB No OTA)**.
4. Upload the provided code.
5. After upload, disconnect **IO0 from GND** and reset the board.

---

## 📡 Wi‑Fi Connection

The ESP32‑CAM will connect to Wi‑Fi using:

```cpp
const char* ssid = "REC";
const char* password = "88888888";
```

Open Serial Monitor at **115200 baud**. You’ll see:

```
WiFi connected
ESP32-CAM IP address: 192.168.x.x
```

---

## 🌐 Viewing the Stream

- Root page:
  ```
  http://<ESP32-CAM-IP>/
  ```
- MJPEG stream:
  ```
  http://<ESP32-CAM-IP>/stream
  ```

---

## ⚠️ Common Errors & Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| **Camera init failed with error 0x20001** | Wrong pin config | Double‑check pin definitions for AI Thinker module |
| **Brownout detector was triggered** | Weak power supply | Use stable 5V ≥1A, avoid weak USB ports |
| **Camera capture failed** | PSRAM not detected/unstable | Select **ESP32 Wrover Module**, use Huge APP partition |
| **Wi‑Fi not connecting** | Wrong SSID/password or weak signal | Verify credentials, move closer to router, use external antenna |
| **Stream slow/freezes** | High resolution or poor Wi‑Fi | Lower resolution (`FRAMESIZE_SVGA` or `FRAMESIZE_VGA`), reduce `jpeg_quality` |
| **Upload fails** | IO0 not grounded during flashing | Connect IO0 → GND before uploading, then disconnect after |

---

## 🔧 Tips

- Use **FRAMESIZE_QVGA** for faster streaming if bandwidth is limited.
- Add external antenna for better Wi‑Fi stability.
- Assign **static IPs** if using multiple ESP32‑CAMs.
- Embed stream in a webpage:
  ```html
  <img src="http://<ESP32-CAM-IP>/stream">
  ```
