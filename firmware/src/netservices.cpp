#include "netservices.h"
#include "config.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "credentials.h"
#include "logger.h"

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

// Register OTA update handlers and start the service
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
