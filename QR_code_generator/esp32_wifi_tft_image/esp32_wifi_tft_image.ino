/*

check with proper driver in ur tft and use

  ESP32 + 3.5" SPI TFT (ILI9488/ILI9486, 480x320) — WiFi Image Display
  ---------------------------------------------------------------------
  Open the ESP32's IP address in your phone's browser, pick a JPEG
  photo from your gallery, tap "Send to Display" — it shows up on
  the TFT.

  LIBRARIES NEEDED (install via Arduino Library Manager):
    - TFT_eSPI        (by Bodmer)
    - TJpg_Decoder    (by Bodmer)

  TFT_eSPI SETUP (required, one-time, before this sketch will compile
  correctly):
    Open: Arduino/libraries/TFT_eSPI/User_Setup.h
    and set it up for your panel. Typical wiring for an ESP32 board
    with a 3.5" ILI9488 SPI TFT:

      #define ILI9488_DRIVER
      #define TFT_MISO 19
      #define TFT_MOSI 23
      #define TFT_SCLK 18
      #define TFT_CS   15
      #define TFT_DC    2
      #define TFT_RST   4
      #define TFT_BL   21        // backlight, if used
      #define SPI_FREQUENCY  40000000

    If your display uses a different controller (ILI9486, ST7796,
    etc.) or different pins, change these to match — check the
    silkscreen on your TFT module or its seller's wiring diagram.

  NOTES:
    - Only JPEG images are supported (TJpg_Decoder). If your phone
      sends HEIC (some iPhones default to this), either switch the
      camera format to "Most Compatible" (JPEG) in iOS settings, or
      just pick/share an existing JPEG.
    - Large photos are fine — they're auto-downscaled to fit the
      screen.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

// ---------- WiFi credentials ----------
const char* ssid     = "REC";
const char* password = "88888888";

TFT_eSPI tft = TFT_eSPI();
WebServer server(80);

File uploadFile;
const char* UPLOAD_PATH = "/image.jpg";

// ---------- JPEG render callback: pushes decoded blocks to the TFT ----------
bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;   // stop feeding blocks once off-screen
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// ---------- Simple upload page ----------
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 TFT Image Sender</title>
  <style>
    body{font-family:sans-serif;text-align:center;margin-top:40px;background:#111;color:#eee}
    input,button{padding:12px;font-size:16px;margin-top:20px;border-radius:8px;border:none}
    button{background:#2d8cf0;color:#fff}
    #status{margin-top:20px;font-size:15px}
  </style>
</head>
<body>
  <h2>Send Photo to TFT</h2>
  <input type="file" id="file" accept="image/jpeg,image/jpg"><br>
  <button onclick="upload()">Send to Display</button>
  <div id="status"></div>
<script>
function upload(){
  const f = document.getElementById('file').files[0];
  if(!f){ document.getElementById('status').innerText='Pick an image first'; return; }
  document.getElementById('status').innerText='Uploading...';
  const fd = new FormData();
  fd.append('image', f, 'image.jpg');
  fetch('/upload', {method:'POST', body: fd})
    .then(r => r.text())
    .then(t => document.getElementById('status').innerText = t)
    .catch(e => document.getElementById('status').innerText = 'Error: '+e);
}
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Called repeatedly while the file streams in
void handleUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    if (LittleFS.exists(UPLOAD_PATH)) LittleFS.remove(UPLOAD_PATH);
    uploadFile = LittleFS.open(UPLOAD_PATH, FILE_WRITE);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
  }
}

// Called once the whole request is done
void handleUploadDone() {
  server.send(200, "text/plain", "Image received, displaying...");
  showImage();
}

void showImage() {
  tft.fillScreen(TFT_BLACK);

  uint16_t w = 0, h = 0;
  TJpgDec.getFsJpgSize(&w, &h, UPLOAD_PATH, LittleFS);
  if (w == 0 || h == 0) {
    tft.setCursor(10, 10);
    tft.setTextColor(TFT_RED);
    tft.println("Could not decode JPEG");
    return;
  }

  // Auto-scale (TJpg_Decoder supports 1/2/4/8) so it fits the panel
  uint8_t scale = 1;
  while ((w / scale) > tft.width() || (h / scale) > tft.height()) {
    scale *= 2;
    if (scale >= 8) { scale = 8; break; }
  }
  TJpgDec.setJpgScale(scale);

  int16_t offX = (tft.width()  - (w / scale)) / 2;
  int16_t offY = (tft.height() - (h / scale)) / 2;

  TJpgDec.drawFsJpg(offX, offY, UPLOAD_PATH, LittleFS);
}

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);           // 0-3: rotate if image looks sideways/upside down
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 10);
  tft.println("Connecting WiFi...");

  if (!LittleFS.begin(true)) {
    tft.println("LittleFS mount failed");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10);
  tft.println("Connected!");
  tft.println(WiFi.localIP());
  Serial.println(WiFi.localIP());

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tftOutput);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_POST, handleUploadDone, handleUpload);
  server.begin();
}

void loop() {
  server.handleClient();
}
