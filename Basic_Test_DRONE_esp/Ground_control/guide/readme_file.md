
🛠️ Hardware Setup
Board: ESP32 (since you’re using <WiFi.h> and WebServer.h).

RF Module: nRF24L01 connected to pins:

CE → GPIO4

CSN → GPIO5

MOSI/MISO/SCK → SPI pins of ESP32.

Power: nRF24L01 needs a stable 3.3V supply (sometimes with a capacitor across VCC–GND for stability).


💻 Software Preparation
Arduino IDE:

Install ESP32 board support (via Board Manager URL: https://dl.espressif.com/dl/package_esp32_index.json).

Select your ESP32 board under Tools → Board.

Libraries:

Install RF24 library.

Ensure WiFi and WebServer are available (they come with ESP32 core).

Upload Code:

Paste your code into Arduino IDE.

Upload to ESP32.

📡 Wi‑Fi Access Point
Your ESP32 will create a Wi‑Fi AP:

SSID: DroneGround

Password: drone1234

Connect your phone/laptop to this Wi‑Fi network.

Open browser → go to 192.168.4.1 (default ESP32 AP IP).

You’ll see the Drone Controller webpage with sliders and buttons.

🎮 Web Controller in Action
Throttle, Roll, Pitch, Yaw sliders: Move them to send values.

Arm/Disarm buttons: Toggle arming state.

The webpage sends commands every 50 ms (20 Hz) to /cmd.

ESP32 parses values, packs them into Ctrl struct, and transmits via RF24.

📡 RF24 Transmission
ESP32 sends the struct {throttle, roll, pitch, yaw, arming} to the drone’s RF24 receiver.

On the drone side, you need another ESP32/Arduino with RF24 configured as a receiver:

cpp
radio.openReadingPipe(1, pipeAddr);
radio.startListening();
if (radio.available()) {
    Ctrl ctrl;
    radio.read(&ctrl, sizeof(ctrl));
    // Use ctrl.throttle, ctrl.roll, etc. to drive motors
}
🔍 Real‑Time Testing Flow
Power up ESP32 (Ground Control).

Connect to Wi‑Fi AP (DroneGround).

Open 192.168.4.1 in browser.

Move sliders → watch Serial Monitor for debug prints.

Verify RF24 receiver gets the struct.

Map values to motor ESCs (PWM outputs) on the drone.

⚠️ Safety Notes
Test first with Serial Monitor only (no motors connected).

Once values are confirmed, connect ESCs/motors carefully.

Secure the drone before arming — it may spin up quickly.
