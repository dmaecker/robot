#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "credentials.h"
#include "logger.h"


const char* HOSTNAME = "robot_control";
const uint8_t RGB_PIN = 38;

unsigned long lastBlink = 0;
unsigned long lastLog = 0;
bool ledState = false;

// Encoder-Pinout
struct EncoderPins { uint8_t a; uint8_t b; const char* name; };
const EncoderPins ENCODERS[4] = {
  {17, 18, "M1"},  // left
  { 8,  9, "M2"},  // left
  {48, 47, "M3"},  // right
  {21, 14, "M4"},  // right
};

volatile int32_t encoderCounts[4] = {0, 0, 0, 0};

template <uint8_t IDX>
void IRAM_ATTR onEncoderA() {
  bool a = digitalRead(ENCODERS[IDX].a);
  bool b = digitalRead(ENCODERS[IDX].b);
  encoderCounts[IDX] += (a == b) ? 1 : -1;
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  WiFi.setHostname(HOSTNAME);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Hostname: %s\n", WiFi.getHostname());
}

void setupOTA() {
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() { LOG("OTA start\n"); });
  ArduinoOTA.onEnd([]()   { LOG("\nOTA end\n"); });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
    LOG("OTA %u%%\r \n \n", (p * 100) / t);
  });
  ArduinoOTA.onError([](ota_error_t e) {
    LOG("OTA error %u\n \n", e);
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

void setupEncoders() {
  for (int i = 0; i < 4; i++) {
    pinMode(ENCODERS[i].a, INPUT);
    pinMode(ENCODERS[i].b, INPUT);
  }
  
  attachInterrupt(ENCODERS[0].a, onEncoderA<0>, CHANGE);
  attachInterrupt(ENCODERS[1].a, onEncoderA<1>, CHANGE);
  attachInterrupt(ENCODERS[2].a, onEncoderA<2>, CHANGE);
  attachInterrupt(ENCODERS[3].a, onEncoderA<3>, CHANGE);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== robot_control boot ===");

  setupWiFi();

  logger_begin();
  LOG("boot complete\n");

  if (MDNS.begin(HOSTNAME)) {
    LOG("mDNS started as robot_control.local\n");
  }

  setupOTA();
  setupEncoders();
  LOG("encoders ready\n");
}

void loop() {
  ArduinoOTA.handle();

  unsigned long now = millis();

  if (now - lastBlink > 1000) {
    lastBlink = now;
    ledState = !ledState;
    neopixelWrite(RGB_PIN, 0, ledState ? 8 : 0, 0);  
  }

  if (now - lastLog > 500) {
    lastLog = now;
    int32_t c[4];
    noInterrupts();
    for (int i = 0; i < 4; i++) c[i] = encoderCounts[i];
    interrupts();
    LOG("ENC M1=%ld M2=%ld M3=%ld M4=%ld\n",
        (long)c[0], (long)c[1], (long)c[2], (long)c[3]);
  }
}