#include "webcontrol.h"
#include "config.h"
#include "motors.h"
#include "webpage.h"
#include "logger.h"

#include <ESPAsyncWebServer.h>

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

// Last received command. Written by the async WS task, read by loop().
static volatile int8_t        g_vx = 0;
static volatile int8_t        g_vy = 0;
static volatile int8_t        g_omega = 0;
static volatile int8_t        g_speed = 100;   // 0..100 %  speed
static volatile unsigned long g_lastCmdMs = 0;

static void onWsEvent(AsyncWebSocket* /*srv*/, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      LOG("ws client %u connected\n", client->id());
      break;

    case WS_EVT_DISCONNECT:
      LOG("ws client disconnected\n");
      // Connection lost: drop the command; failsafe stops the motors.
      g_vx = g_vy = g_omega = 0;
      break;

    case WS_EVT_DATA: {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      // Expect a binary frame: [vx, vy, omega, speed].
      if (info->final && info->index == 0 && info->len == len &&
          info->opcode == WS_BINARY && len >= 4) {
        g_vx    = (int8_t)data[0];
        g_vy    = (int8_t)data[1];
        g_omega = (int8_t)data[2];
        int8_t s = (int8_t)data[3];
        if (s < 0)   s = 0;
        if (s > 100) s = 100;
        g_speed = s;
        g_lastCmdMs = millis();
      }
      break;
    }

    default:
      break;
  }
}

void setupWebControl() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", INDEX_HTML);
  });

  server.begin();
  LOG("web control ready (http + /ws)\n");
}

void webControlLoop() {
  ws.cleanupClients();

  static unsigned long lastDrive = 0;
  unsigned long now = millis();
  if (now - lastDrive < DRIVE_UPDATE_MS) return;
  lastDrive = now;

  if (now - g_lastCmdMs > FAILSAFE_TIMEOUT_MS) {
    motorsStop();
  } else {
    int scale = g_speed;  // 0..100
    driveRobot(g_vx * scale / 100, g_vy * scale / 100, g_omega * scale / 100);
  }
}