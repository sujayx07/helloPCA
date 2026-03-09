#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// --------------------- CONFIGURATION ---------------------
const char* ssid = "Wokwi-GUEST";
const char* password = "";

String apiKey = "6Y4P1DYZEFRYBVQF";
const char* server = "http://api.thingspeak.com/update";

// --------------------- COMPONENT PINS ---------------------
#define LED_GREEN 27
#define LED_YELLOW 26
#define LED_RED 25
#define BUZZER 14
#define BUTTON 33
#define POTENTIOMETER 34

Adafruit_MPU6050 mpu;
bool accidentDetected = false;

// --------------------- WIFI + THINGSPEAK ---------------------
void sendDataToThingSpeak(float accel, int vibrationLevel, int condition) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = server;
    url += "?api_key=" + apiKey;
    url += "&field1=" + String(accel);
    url += "&field2=" + String(vibrationLevel);
    url += "&field3=" + String(condition);

    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("📡 Data sent to ThingSpeak successfully!");
    } else {
      Serial.println("⚠️ Error sending data!");
    }
    http.end();
  }
}

// --------------------- SETUP ---------------------
void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  if (!mpu.begin()) {
    Serial.println("❌ Could not find MPU6050 sensor!");
    while (1);
  }

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Connected!");
}

// --------------------- LOOP ---------------------
void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float accel = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
  int vibrationLevel = analogRead(POTENTIOMETER);
  int roadCondition = 0;

  if (accel > 25.0) {
    accidentDetected = true;
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
    tone(BUZZER, 1000);
    roadCondition = 3; // Accident
  } else if (vibrationLevel > 2500) {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_GREEN, LOW);
    noTone(BUZZER);
    roadCondition = 2; // Rough
  } else {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, HIGH);
    noTone(BUZZER);
    roadCondition = 1; // Smooth
  }

  if (digitalRead(BUTTON) == LOW) {
    accidentDetected = false;
    noTone(BUZZER);
    Serial.println("🔁 System Reset by Button");
    delay(1000);
  }

  sendDataToThingSpeak(accel, vibrationLevel, roadCondition);

  Serial.print("Accel: "); Serial.print(accel);
  Serial.print(" | Vibration: "); Serial.print(vibrationLevel);
  Serial.print(" | Condition: "); Serial.println(roadCondition);

  delay(15000); // ThingSpeak rate limit
}
