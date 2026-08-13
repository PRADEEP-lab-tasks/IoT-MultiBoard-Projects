# ⚡ ESP32 Function Generator (DAC‑based)

This project turns an **ESP32** into a simple function generator using its built‑in DAC (Digital‑to‑Analog Converter). It can output **sine, triangle, sawtooth, and square waves** on GPIO25 (DAC1).

---

## 🎯 Features
- Waveforms: **Sine, Triangle, Sawtooth, Square**
- Frequency control via **Serial Monitor commands**
- Adjustable amplitude (0–255 DAC value)
- Output range: **0–3.3V**
- Sample rate: **40 kHz**

---

## 🔌 Hardware Setup
- ESP32 board (with DAC pins available: GPIO25 or GPIO26)
- Connect **GPIO25** to your oscilloscope or ADC input
- Optional: add a small series resistor (100–220Ω) for protection

---

## 🛠️ Software Setup
1. Install **Arduino IDE** with ESP32 board support:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
2. Select **Board: ESP32 Dev Module** (or your specific ESP32 board).
3. Copy the provided code into Arduino IDE.
4. Upload to your ESP32.

---

## 📡 Serial Commands
Use the **Serial Monitor (115200 baud)** to control the generator:

- `s <freq>` → Sine wave (e.g., `s 1000` for 1 kHz)
- `t <freq>` → Triangle wave
- `w <freq>` → Sawtooth wave
- `q <freq>` → Square wave
- `a <0-255>` → Set amplitude (max DAC value)

Example:
```
s 500
a 200
```
Generates a 500 Hz sine wave with amplitude scaled to 200/255.

---

## ⚠️ Common Issues & Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| **No waveform output** | DAC not enabled | Ensure `dac_output_enable(DAC_CHANNEL_1)` is called |
| **Distorted signal** | Frequency too high for sample rate | Keep frequency ≤ 10 kHz for clean output |
| **Upload fails** | Wrong board selected | Use **ESP32 Dev Module** or correct variant |
| **Serial commands ignored** | Baud mismatch | Set Serial Monitor to **115200 baud** |
| **Square wave unstable** | Phase increment too small | Increase frequency or check ISR logic |

---

## 🔧 Tips
- For testing, connect GPIO25 → GPIO34 (ADC) to measure with another ESP32.
- Use a real oscilloscope for accurate waveform visualization.
- Adjust `sampleRateHz` if you need higher frequency support (trade‑off: resolution vs. speed).
- Amplitude scaling is linear, but DAC resolution is only 8‑bit (0–255).

---

## 📜 License
Open‑source project for learning and experimentation. Use freely for educational purposes.

---

