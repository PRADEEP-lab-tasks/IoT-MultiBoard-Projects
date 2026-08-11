
# 🗑️ Smart Dustbin using Arduino

This project demonstrates a **Smart Dustbin** that automatically opens its lid when someone approaches.  
It uses an **Ultrasonic sensor** to detect distance and a **Servo motor** to open/close the lid.  
An LED indicator is also included to show activity.

---

## 📌 Features
- Hands-free operation: lid opens automatically when someone is near.
- Ultrasonic sensor detects distance up to ~50 cm.
- Servo motor controls the lid movement.
- LED indicator lights up when the dustbin is active.
- Simple and cost-effective design.

---

## 🛠️ Hardware Requirements
- Arduino Uno / Nano / Mega
- Ultrasonic Sensor (HC-SR04)
- Servo Motor (SG90 or similar)
- LED + Resistor
- Jumper wires
- Breadboard
- Power supply

---

## 📂 Code Overview
```cpp
#include <Servo.h>   // Servo library

Servo servo;    
int trigPin = 5;   
int echoPin = 6;  
int servoPin = 7;
int led = 10;

long duration, dist, average;  
long aver[3];   // array for averaging distance

void setup() {      
    Serial.begin(9600);
    servo.attach(servoPin); 
    pinMode(trigPin, OUTPUT); 
    pinMode(echoPin, INPUT); 
    servo.write(0);         // close lid on startup
    delay(100);
    servo.detach();
}

void measure() { 
    digitalWrite(led, HIGH);
    digitalWrite(trigPin, LOW);
    delayMicroseconds(5);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(15);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    dist = (duration / 2) / 50.1;    // calculate distance
}

void loop() {
    for (int i = 0; i <= 2; i++) {   // average distance
        measure();              
        aver[i] = dist;           
        delay(10);              
    }
    dist = (aver[0] + aver[1] + aver[2]) / 3;   

    if (dist < 50) {  // threshold distance
        servo.attach(servoPin);
        delay(1);
        servo.write(0);       // open lid
        delay(3000);      
        servo.write(150);     // close lid
        delay(1000);
        servo.detach();     
    }
    Serial.print(dist);
}
