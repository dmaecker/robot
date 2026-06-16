#include "comm.h"
#include "config.h"
#include "motors.h"
#include "encoders.h"
#include "logger.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#define PKT_CMD   0x01
#define PKT_TELEM 0x02
#define PKT_HELLO 0x03

static WiFiUDP udp;
static const uint16_t LOCAL_PORT = 8889;   
static const uint16_t PI_PORT    = 8890;  

static IPAddress piAddr;
static bool      piKnown   = false;
static uint32_t  lastCmdMs = 0;
static uint32_t  lastAnnounceMs = 0;

void setupComm() {
  udp.begin(LOCAL_PORT);
  LOG("comm udp on :%u\n", LOCAL_PORT);
}

// Broadcast a initial packet so the Pi can learn the address without
// static IPs or mDNS. Stops once the Pi is known.
static void commAnnounce() {
  if (piKnown) return;

  uint32_t now = millis();
  if (now - lastAnnounceMs < 1000) return;   
  lastAnnounceMs = now;

  IPAddress bcast = WiFi.localIP();
  bcast[3] = 255;                            // subnet broadcast

  uint8_t hello = PKT_HELLO;
  udp.beginPacket(bcast, PI_PORT);
  udp.write(&hello, 1);
  udp.endPacket();
}

void commPoll() {
  commAnnounce();

  int sz = udp.parsePacket();
  if (sz >= 7) {
    uint8_t buf[7];
    udp.read(buf, 7);
    if (buf[0] == PKT_CMD) {
      int16_t vx, vy, omega;
      memcpy(&vx,    buf + 1, 2);
      memcpy(&vy,    buf + 3, 2);
      memcpy(&omega, buf + 5, 2);

      piAddr    = udp.remoteIP();   // learn / refresh Pi address
      piKnown   = true;
      lastCmdMs = millis();

      driveRobot(vx, vy, omega);
    }
  }

  // Failsafe: no command within timeout -> stop
  if (piKnown && millis() - lastCmdMs > FAILSAFE_TIMEOUT_MS) {
    motorsStop();
  }
}

void commSendTelemetry() {
  if (!piKnown) return;

  int32_t c[4];
  noInterrupts();
  for (int i = 0; i < 4; i++) c[i] = encoderCounts[i];
  interrupts();

  uint32_t ts = millis();

  uint8_t buf[21];
  buf[0] = PKT_TELEM;
  memcpy(buf + 1,  c,   16);
  memcpy(buf + 17, &ts,  4);

  udp.beginPacket(piAddr, PI_PORT);
  udp.write(buf, 21);
  udp.endPacket();
}