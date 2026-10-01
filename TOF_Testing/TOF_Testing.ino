#include <ESP32Servo.h>
#include "esp_system.h"

#define SERVO_PIN 18
Servo myServo;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Booting...");
  Serial.print("Reset reason: ");
  Serial.println(esp_reset_reason());

  myServo.attach(SERVO_PIN, 500, 2500);
  myServo.write(90);
  delay(1000);

  Serial.println("Start continuous rotation");
}

void loop() {
  myServo.write(180);   // moderate forward
  Serial.println("Running...");
  delay(1000);
}