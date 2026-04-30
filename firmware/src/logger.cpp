#include "logger.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <stdarg.h>

static WiFiUDP udp;
static uint16_t udpPort = 4444;
static IPAddress broadcastIP;
static bool initialized = false;

void logger_begin(uint16_t port) {
  udpPort = port;
  IPAddress ip   = WiFi.localIP();
  IPAddress mask = WiFi.subnetMask();
  for (int i = 0; i < 4; i++) {
    broadcastIP[i] = (ip[i] & mask[i]) | (~mask[i] & 0xFF);
  }
  udp.begin(udpPort);
  initialized = true;
}

void logger_log(const char* fmt, ...) {
  char buf[256];
  int n = snprintf(buf, sizeof(buf), "[%lu][robot_control] ", millis());

  va_list args;
  va_start(args, fmt);
  int n2 = vsnprintf(buf + n, sizeof(buf) - n, fmt, args);
  va_end(args);

  if (n2 < 0) return;
  int len = n + n2;
  if (len >= (int)sizeof(buf)) len = sizeof(buf) - 1;

  Serial.write((uint8_t*)buf, len);

  if (initialized && WiFi.status() == WL_CONNECTED) {
    udp.beginPacket(broadcastIP, udpPort);
    udp.write((uint8_t*)buf, len);
    udp.endPacket();
  }
}