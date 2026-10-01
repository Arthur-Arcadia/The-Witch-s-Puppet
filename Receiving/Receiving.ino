#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

#define SERVO_PIN 18
#define TOP_DISTANCE 80
#define BOTTOM_DISTANCE 50
#define ACTION_INTERVAL 1000 

Servo myServo;

typedef struct {
  int distance;
  uint8_t sensorID; 
} SensorData;

int distances[4] = {999, 999, 999, 999};                 // [UPDATED]
unsigned long lastSeen[4] = {0, 0, 0, 0};  

bool isMoving = false;
unsigned long lastActionTime = 0; 

//Receive data from sending port
void onReceive(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  SensorData incoming;   // [UPDATED]
  memcpy(&incoming, incomingData, sizeof(incoming));   // [UPDATED]

  // [UPDATED] Validate sensor ID before storing
  if (incoming.sensorID >= 1 && incoming.sensorID <= 4) {
    int index = incoming.sensorID - 1;   // [UPDATED]

    distances[index] = incoming.distance;   // [UPDATED]
    lastSeen[index] = millis();            // [UPDATED]

    // [UPDATED] Debug print
    Serial.print("Sensor ");
    Serial.print(incoming.sensorID);
    Serial.print(": ");
    Serial.print(incoming.distance);
    Serial.println(" cm");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

//Servo initialization
  myServo.attach(SERVO_PIN, 500, 2500);
  myServo.write(0);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);

  Serial.print("Receiver MAC: ");   // [UPDATED]
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);
}

void loop() {
  unsigned long now = millis();

  // [UPDATED] Timeout check for offline sensors
  for (int i = 0; i < 4; i++) {
    if (now - lastSeen[i] > 1000) {   // [UPDATED]
      distances[i] = 999;            // [UPDATED]
    }
  }

  // [UPDATED] Check all 4 sensors
  // Trigger if ANY sensor is close enough
  bool triggered = false;   // [UPDATED]

  for (int i = 0; i < 4; i++) {
    if (distances[i] < TOP_DISTANCE && distances[i] > BOTTOM_DISTANCE) {   // [UPDATED]
      triggered = true;                      // [UPDATED]
      break;
    }
  }

//Determines when to move
  if (triggered) {   // [UPDATED]
    if (!isMoving && now - lastActionTime > ACTION_INTERVAL) {
      isMoving = true;
      lastActionTime = now;
    }
  }

  if (isMoving) {
    Serial.println("Servo Triggered");   // [UPDATED]

    myServo.write(150);   // fast forward strike
    delay(720);           // swing out (~180°+)

    myServo.write(90);    // stop briefly
    delay(80);

    myServo.write(30);    // fast reverse reset
    delay(740);           // return to start

    myServo.write(90);    // stop

    isMoving = false;
    lastActionTime = millis();
  }
}