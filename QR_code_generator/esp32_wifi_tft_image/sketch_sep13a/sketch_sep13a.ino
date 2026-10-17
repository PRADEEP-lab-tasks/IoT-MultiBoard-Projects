//this code only shows an one image that u want to use

#include <TFT_eSPI.h>
#include <SPI.h>
#include "myimage.h"

TFT_eSPI tft = TFT_eSPI();

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);

  int x = (480 - qr_image_width) / 2;
  int y = (320 - qr_image_height) / 2;

  tft.setSwapBytes(true);   // fixes color order if colors look wrong
  tft.pushImage(x, y, qr_image_width, qr_image_height, qr_image);
}

void loop() {
}
