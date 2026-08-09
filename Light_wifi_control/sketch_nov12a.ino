#define BLYNK_TEMPLATE_ID "TMPL3pimTr7ce"
#define BLYNK_TEMPLATE_NAME "home automation"
#define BLYNK_AUTH_TOKEN "NazZz9TvcwxmzV36mIN3cMDu1hayalmQ"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char ssid[] = "GOVIND";
char pass[] = "12341234";

#define RELAY_PIN 5   // GPIO5 (D1 on NodeMCU)

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

BLYNK_WRITE(V0) {
  int relayState = param.asInt();
  digitalWrite(RELAY_PIN, relayState);
}

void loop() {
  Blynk.run();
}