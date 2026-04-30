#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "credentials.h"
#include "logger.h"


const char* HOSTNAME = "robot_control";
const uint8_t LED_PIN = 2;

unsigned long lastBlink = 0;
bool ledState = false;

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

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  delay(500);
  Serial.println("\n=== robot_control boot ===");

  setupWiFi();

  logger_begin();
  LOG("boot complete\n");

  if (MDNS.begin(HOSTNAME)) {
    LOG("mDNS started as robot_control.local\n");
  }

  setupOTA();
}

void loop() {
  ArduinoOTA.handle();

  unsigned long now = millis();
  if (now - lastBlink > 1000) {
    lastBlink = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
}