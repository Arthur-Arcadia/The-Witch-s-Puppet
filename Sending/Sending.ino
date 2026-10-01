#include <WiFi.h>
#include <esp_now.h>
#include <FastLED.h>

// =========================
// ESP32-S3-Zero Pin Config
// =========================
#define LED_PIN 21          // Onboard WS2812 RGB LED on ESP32-S3-Zero
#define NUM_LEDS 1          // Only 1 onboard RGB LED

#define TRIG_PIN 5
#define ECHO_PIN 6

#define SENSOR_ID 3 // Different on each ESP32

CRGB leds[NUM_LEDS];

// Receiver MAC address
uint8_t receiverAddress[] = {0xC8, 0xF0, 0x9E, 0x9B, 0x6A, 0x1C};
//c8:f0:9e:9b:6a:1c

typedef struct {
  int distance;
  uint8_t sensorID;
} SensorData;

SensorData data;

// =========================
// Ultrasonic Distance Read
// =========================
float getDistanceCM() {
  long duration;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000);  // timeout 30ms

  if (duration == 0) return -1;

  return duration * 0.034 / 2;
}

// =========================
// ESP-NOW Send Callback
// =========================
void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  // Optional debug
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("BOOTING...");

  // =========================
  // LED Init
  // =========================
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  FastLED.clear();
  FastLED.show();   // LED off at boot

  // =========================
  // Ultrasonic Init
  // =========================
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("Ultrasonic Sensor OK");

  // =========================
  // ESP-NOW Init
  // =========================
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  Serial.println("ESP-NOW OK");

  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Sender ready");
}

void loop() {
  float distance = getDistanceCM();

  if (distance > 0) {

    // =========================
    // In Trigger Range
    // =========================
    if (distance > 50 && distance < 80) {

      // LED ON
      leds[0] = CRGB::Red;
      FastLED.show();

      // Prepare data
      data.distance = (int)distance;
      data.sensorID = SENSOR_ID;

      // Send ONLY when triggered
      esp_now_send(receiverAddress, (uint8_t *)&data, sizeof(data));

      // Serial Output
      Serial.print("Sensor ");
      Serial.print(SENSOR_ID);
      Serial.print("; Distance: ");
      Serial.print(distance);
      Serial.println(" cm -> triggered");

    } else {

      // =========================
      // Out of Trigger Range
      // =========================

      // LED OFF
      leds[0] = CRGB::Black;
      FastLED.show();

      // Serial Output ONLY
      Serial.print("Sensor ");
      Serial.print(SENSOR_ID);
      Serial.print("; Distance: ");
      Serial.print(distance);
      Serial.println(" cm");
    }

  } else {

    // No reading
    leds[0] = CRGB::Black;
    FastLED.show();

    Serial.print("Sensor ");
    Serial.print(SENSOR_ID);
    Serial.println("; Out of range");
  }

  delay(260 + random(0, 80));
}