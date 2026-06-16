#include <Arduino.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#include "logger.h"
#include "config.h"
#include "motors.h"
#include "encoders.h"
#include "netservices.h"
#include "webcontrol.h"
#include "comm.h"

unsigned long lastBlink = 0;
unsigned long lastLog = 0;
bool ledState = false;

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

  setupMotors();
  LOG("motors ready\n");

  setupWebControl();

  setupComm();
  LOG("comm ready\n");

  // startup spin for testing
  LOG("startup spin 1s\n");
  motorM1(true, START_DUTY);
  motorM2(true, START_DUTY);
  motorM3(true, START_DUTY);
  motorM4(true, START_DUTY);
  delay(200);
  motorsStop();
  LOG("startup spin done\n");
}

// Service OTA, blink LED and log encoder counts once per second
void loop() {

  ArduinoOTA.handle();

  commPoll();

  webControlLoop();

  unsigned long now = millis();

  // LED toggle
  if (now - lastBlink > 1000) {
    lastBlink = now;
    ledState = !ledState;
    neopixelWrite(RGB_PIN, 0, ledState ? 8 : 0, 0);
  }

  // log encoder counts
  if (now - lastLog > 1000) {
    lastLog = now;
    int32_t c[4];
    noInterrupts();
    for (int i = 0; i < 4; i++) c[i] = encoderCounts[i];
    interrupts();
    LOG("ENC M1=%ld M2=%ld M3=%ld M4=%ld\n",
        (long)c[0], (long)c[1], (long)c[2], (long)c[3]);
  }

  // send telemetry data in intervals
  static unsigned long lastTelem = 0;
  if (now - lastTelem > 50) {     // 20 Hz
    lastTelem = now;
    commSendTelemetry();
  }
}