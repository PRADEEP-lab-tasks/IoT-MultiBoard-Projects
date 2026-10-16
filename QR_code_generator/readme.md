QR Code Generator using ESP32 and 3.5" TFT Display

A simple and interactive QR Code Generator built with ESP32 and a 3.5-inch TFT Touch Display. This project allows users to generate and display QR codes directly on the TFT screen from custom text, URLs, Wi-Fi credentials, contact information, and other supported data formats.

📌 Features
Generate QR codes from user-defined text
Display QR codes on a 3.5" TFT LCD
Fast QR code rendering using ESP32
Touchscreen-based user interface (optional)
Supports URLs, text messages, Wi-Fi credentials, and contact details
Standalone operation without a PC
Easy to customize and extend
🛠 Hardware Requirements
Component	QuantityESP32 Development Board	1
3.5" TFT Display (ILI9486 / ILI9488 / HX8357 compatible)	1
Touch Controller (XPT2046, if touch-enabled)	1
Jumper Wires	As Required
USB Cable	1
📚 Software Requirements
Arduino IDE 2.x
ESP32 Board Package
TFT_eSPI Library
QRCode Library
SPI Library
📦 Required Libraries

Install the following libraries from the Arduino Library Manager:

TFT_eSPI
QRCode
SPI
XPT2046_Touchscreen (Optional)

🔌 Wiring Connections
ESP32 to TFT Display
TFT Pin	ESP32 PinVCC	3.3V
GND	GND
CS	GPIO 15
RST	GPIO 4
DC	GPIO 2
MOSI	GPIO 23
MISO	GPIO 19
SCK	GPIO 18
LED	3.3V

Note: Pin assignments may vary depending on your TFT module. Update them in the code accordingly.

📂 Project Structure
QR_Code_Generator/
│
├── src/
│   ├── main.ino
│   ├── qr_generator.cpp
│   └── qr_generator.h
│
├── images/
│   └── demo.jpg
│
├── README.md
│
└── LICENSE

🚀 Getting Started
1. Clone the Repository
git clone https://github.com/yourusername/QR_Code_Generator.git
cd QR_Code_Generator

2. Install Dependencies

Open Arduino IDE and install all required libraries.

3. Configure TFT_eSPI

Edit:

Documents/Arduino/libraries/TFT_eSPI/User_Setup.h


Configure the display driver and ESP32 pins for your TFT module.

4. Upload the Code
Select ESP32 Dev Module
Choose the correct COM port
Compile and Upload
🖥 Example
String qrData = "https://github.com";
generateQRCode(qrData);


The generated QR code will be rendered directly on the TFT display.

📷 Project Demo
QR Code Display
+------------------------+
|                        |
|      ███████████       |
|      █ ▄▄▄▄▄ █         |
|      █ █   █ █         |
|      █ ▀▀▀▀▀ █         |
|      ███████████       |
|                        |
+------------------------+

🔄 Future Improvements
Wi-Fi QR code creator
QR code scanner using ESP32-CAM
Save QR images to SD card
Web interface for remote QR generation
Multi-language UI support
Touch keyboard for text input

🎯 Applications
Smart Home Access Sharing
Wi-Fi Credential Sharing
Digital Business Cards
Attendance Systems
Product Information Displays
Contactless Information Sharing

🤝 Contributing

Contributions, issues, and feature requests are welcome.

Fork the repository
Create a feature branch
git checkout -b feature-name

Commit changes
git commit -m "Added new feature"

Push to GitHub
git push origin feature-name

Open a Pull Request
📜 License

This project is licensed under the MIT License.

👨‍💻 Author

Pradeep

GitHub: https://github.com/PRADEEP-lab-tasks

⭐ If you find this project useful, consider giving the repository a star!
