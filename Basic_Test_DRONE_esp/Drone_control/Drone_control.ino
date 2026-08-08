// created by Pradeep

#define BLYNK_TEMPLATE_ID "TMPL3MMYWEOnz"
#define BLYNK_TEMPLATE_NAME "Drone"

#include <ESP8266WiFi.h>

#include <BlynkSimpleEsp8266.h>


char auth[] = "AS_jAbrqnxmtrB5oGYL4t0kDLosPGpVU";
char ssid[] = "ssid";
char pass[] = "password";

int speedValue = 10; // Speed variable

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, pass);
    Blynk.begin(auth, ssid, pass);


    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
}

void loop() {
    Blynk.run();
}

BLYNK_WRITE(V1) { 
    int roll = param.asInt();
    Serial.print("Roll: "); Serial.println(roll);
}
BLYNK_WRITE(V2) { 
    int pitch = param.asInt();
    Serial.print("Pitch: "); Serial.println(pitch);
}
BLYNK_WRITE(V3) { 
    int yaw = param.asInt();
    Serial.print("Yaw: "); Serial.println(yaw);
}
BLYNK_WRITE(V4) { 
    int increment = param.asInt(); 
    speedValue += increment; // Increase speed
    Serial.print("Speed: "); Serial.println(speedValue);
}
