/*
  ESP32 + 3.5" SPI TFT (ILI9488, 480x320)
  Type an amount on a web page  ->  a UPI payment QR code appears on the display.

  Files that must sit together in ONE folder named  esp32_qr_display :
    esp32_qr_display.ino   (this file)
    qrcode_rm.h, qrcode_rm.c   (QR generator by Richard Moore / Project Nayuki, MIT licence.
                                Bundled under a different name because the ESP32 core ships
                                its own "qrcode.h" that would otherwise shadow the library.)

  Library to install (Arduino IDE > Sketch > Include Library > Manage Libraries):
    - TFT_eSPI   by Bodmer   (configured with the supplied User_Setup.h: pins + ILI9488 driver)
  Board: "ESP32 Dev Module"  (package "esp32" by Espressif Systems)

  1) Edit the settings below (WiFi + UPI details)
  2) Upload
  3) Open the IP address shown on the display from a phone/PC on the same WiFi
*/

#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>
#include "qrcode_rm.h"

// ==================== EDIT THESE ====================
const char* WIFI_SSID     = "REC";     // must be a 2.4 GHz network
const char* WIFI_PASSWORD = "88888888";

const char* UPI_ID     = "annamalaiarun396@okaxis";          // your UPI ID (VPA)
const char* PAYEE_NAME = "POWEREX";               // letters, numbers and spaces only

const float MAX_AMOUNT = 100000.0;                // largest amount the web page accepts
// ====================================================

// Byte-mode capacity (characters) of QR versions 1..10 at error-correction level M.
// The sketch picks the smallest version that fits, so the modules stay as big as possible.
const uint8_t QR_CAPACITY[]  = {14, 26, 42, 62, 84, 106, 122, 152, 180, 213};
const uint8_t QR_MAX_VERSION = sizeof(QR_CAPACITY);

TFT_eSPI  tft = TFT_eSPI();
WebServer server(80);

// ---------------------------------------------------------------------------
// Text that goes inside the QR code
// ---------------------------------------------------------------------------
String urlEncodeSpaces(String s) {
  s.replace(" ", "%20");
  return s;
}

// UPI deep link. Any UPI app (GPay, PhonePe, Paytm, BHIM...) understands it.
// Want a different QR content (plain amount, a URL...)? Just return that string here.
String buildPayString(const String &amount) {
  String s = "upi://pay?pa=" + String(UPI_ID);
  s += "&pn=" + urlEncodeSpaces(PAYEE_NAME);
  s += "&am=" + amount;
  s += "&cu=INR";
  return s;
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------
void showMessage(const String &line1, const String &line2, uint8_t size2 = 2) {
  tft.fillScreen(TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString(line1, tft.width() / 2, tft.height() / 2 - 24);
  tft.setTextSize(size2);
  tft.drawString(line2, tft.width() / 2, tft.height() / 2 + 20);
}

void showIdleScreen() {
  showMessage("Open in a browser:", WiFi.localIP().toString(), 3);
}

void showQRCode(const String &data, const String &amount) {
  // smallest QR version that can hold the text
  uint8_t version = 0;
  for (uint8_t v = 1; v <= QR_MAX_VERSION; v++) {
    if (data.length() <= QR_CAPACITY[v - 1]) { version = v; break; }
  }
  if (version == 0) {
    showMessage("QR text too long", "Shorten name / UPI ID");
    return;
  }

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(QR_MAX_VERSION)];
  qrcode_initText(&qrcode, qrcodeData, version, ECC_MEDIUM, data.c_str());

  const int W = tft.width();     // 480 in landscape
  const int H = tft.height();    // 320 in landscape

  // module size (pixels) so the QR + a 4-module white border fits the screen height
  int m = H / (qrcode.size + 8);
  if (m < 1) m = 1;
  const int quiet  = 4 * m;                          // white "quiet zone" every scanner needs
  const int cardPx = qrcode.size * m + 2 * quiet;
  const int cardX  = 6;
  const int cardY  = (H - cardPx) / 2;

  tft.fillScreen(TFT_NAVY);
  tft.fillRect(cardX, cardY, cardPx, cardPx, TFT_WHITE);

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(cardX + quiet + x * m, cardY + quiet + y * m, m, m, TFT_BLACK);
      }
    }
  }

  // right-hand side: label + amount
  const int tx = cardX + cardPx + (W - cardX - cardPx) / 2;
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(2);
  tft.drawString("SCAN & PAY", tx, H / 2 - 50);
  tft.setTextColor(TFT_YELLOW, TFT_NAVY);
  tft.drawString("Rs.", tx, H / 2 - 12);
  tft.setTextSize(amount.length() > 7 ? 2 : 3);
  tft.drawString(amount, tx, H / 2 + 26);
}

// ---------------------------------------------------------------------------
// Web pages
// ---------------------------------------------------------------------------
void sendPage(const String &body) {
  String page =
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>QR Display</title><style>"
    "body{font-family:Arial,sans-serif;text-align:center;margin-top:40px;background:#f2f2f2}"
    "input[type=number]{font-size:28px;padding:10px;width:220px;text-align:center}"
    ".btn{display:inline-block;font-size:20px;padding:12px 26px;margin:12px 6px;border:0;"
    "border-radius:6px;background:#28a745;color:#fff;text-decoration:none}"
    ".red{background:#c0392b}"
    "</style></head><body>" + body + "</body></html>";
  server.send(200, "text/html", page);
}

void handleRoot() {
  sendPage(
    "<h2>Enter Amount (Rs.)</h2>"
    "<form action='/setamount' method='GET'>"
    "<input type='number' name='amount' step='0.01' min='0.01' placeholder='e.g. 250' required autofocus><br>"
    "<input class='btn' type='submit' value='Show QR on Display'>"
    "</form>"
    "<a class='btn red' href='/clear'>Clear Display</a>");
}

void handleSetAmount() {
  float amt = server.arg("amount").toFloat();          // empty / junk -> 0
  if (amt <= 0 || amt > MAX_AMOUNT) {
    sendPage("<h3>Enter an amount between 0.01 and " + String(MAX_AMOUNT, 0) + "</h3>"
             "<a class='btn' href='/'>Back</a>");
    return;
  }

  String amount = String(amt, 2);                      // e.g. "250.00"
  showQRCode(buildPayString(amount), amount);

  sendPage("<h2>QR shown for Rs. " + amount + "</h2>"
           "<a class='btn' href='/'>New amount</a>"
           "<a class='btn red' href='/clear'>Clear Display</a>");
}

void handleClear() {
  showIdleScreen();
  sendPage("<h2>Display cleared</h2><a class='btn' href='/'>Enter amount</a>");
}

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);            // landscape 480x320 (use 3 if the picture is upside-down)
  showMessage("Connecting to WiFi...", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.print("\nConnected. Open http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/setamount", handleSetAmount);
  server.on("/clear", handleClear);
  server.begin();

  showIdleScreen();
}

void loop() {
  server.handleClient();
}
