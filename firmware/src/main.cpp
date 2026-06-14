#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "credentials.h"
#include "logger.h"


const char* HOSTNAME = "robot_control";
const uint8_t RGB_PIN = 38; // onboard RGB LED

unsigned long lastBlink = 0;
unsigned long lastLog = 0;
bool ledState = false;

// Encoder pinout: {A, B, name} per motor
struct EncoderPins { uint8_t a; uint8_t b; const char* name; };
const EncoderPins ENCODERS[4] = {
  {17, 18, "M1"}, 
  { 8,  9, "M2"},  
  {48, 47, "M3"},  
  {21, 14, "M4"},  
};

// LEDC channels (one per motor)
const int M1_CH = 0;
const int M2_CH = 1;
const int M3_CH = 2;
const int M4_CH = 3;
const int PWM_FREQ = 20000;
const int PWM_RES  = 8;         // 8-bit duty (0..255)
const uint8_t START_DUTY = 80;  // startup test duty

// === Left driver (M1 + M2) ===
const uint8_t L_PWMA = 4;   // M1
const uint8_t L_AIN1 = 6;
const uint8_t L_AIN2 = 5;
const uint8_t L_PWMB = 16;  // M2
const uint8_t L_BIN1 = 7;
const uint8_t L_BIN2 = 15;

// === Right driver (M3 + M4) ===
const uint8_t R_PWMA = 40;  // M3
const uint8_t R_AIN1 = 41;
const uint8_t R_AIN2 = 39;
const uint8_t R_PWMB = 1;   // M4
const uint8_t R_BIN1 = 42;
const uint8_t R_BIN2 = 2;

volatile int32_t encoderCounts[4] = {0, 0, 0, 0};

// Per-motor encoder ISR; counts up/down from A==B on channel-A edge
template <uint8_t IDX>
void IRAM_ATTR onEncoderA() {
  bool a = digitalRead(ENCODERS[IDX].a);
  bool b = digitalRead(ENCODERS[IDX].b);
  encoderCounts[IDX] += (a == b) ? 1 : -1;
}


// Init direction pins and PWM channels, leave all motors stopped
void setupMotors() {

  // Direction pins as outputs
  pinMode(L_AIN1, OUTPUT); pinMode(L_AIN2, OUTPUT);
  pinMode(L_BIN1, OUTPUT); pinMode(L_BIN2, OUTPUT);
  pinMode(R_AIN1, OUTPUT); pinMode(R_AIN2, OUTPUT);
  pinMode(R_BIN1, OUTPUT); pinMode(R_BIN2, OUTPUT);

  // Configure PWM channels
  ledcSetup(M1_CH, PWM_FREQ, PWM_RES);
  ledcSetup(M2_CH, PWM_FREQ, PWM_RES);
  ledcSetup(M3_CH, PWM_FREQ, PWM_RES);
  ledcSetup(M4_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(L_PWMA, M1_CH);
  ledcAttachPin(L_PWMB, M2_CH);
  ledcAttachPin(R_PWMA, M3_CH);
  ledcAttachPin(R_PWMB, M4_CH);

  // Start stopped
  ledcWrite(M1_CH, 0);
  ledcWrite(M2_CH, 0);
  ledcWrite(M3_CH, 0);
  ledcWrite(M4_CH, 0);
}

// Drive left-front motor: direction + duty
void motorM1(bool fwd, uint8_t duty) {
  digitalWrite(L_AIN1, fwd ? HIGH : LOW);
  digitalWrite(L_AIN2, fwd ? LOW  : HIGH);
  ledcWrite(M1_CH, duty);
}

// Drive left-rear motor: direction + duty
void motorM2(bool fwd, uint8_t duty) {
  digitalWrite(L_BIN1, fwd ? HIGH : LOW);
  digitalWrite(L_BIN2, fwd ? LOW  : HIGH);
  ledcWrite(M2_CH, duty);
}

// Drive right-front motor; polarity inverted (right side mirrored)
void motorM3(bool fwd, uint8_t duty) {
  digitalWrite(R_AIN1, fwd ? LOW  : HIGH);
  digitalWrite(R_AIN2, fwd ? HIGH : LOW);
  ledcWrite(M3_CH, duty);
}

// Drive right-rear motor; polarity inverted (right side mirrored)
void motorM4(bool fwd, uint8_t duty) {
  digitalWrite(R_BIN1, fwd ? LOW  : HIGH);
  digitalWrite(R_BIN2, fwd ? HIGH : LOW);
  ledcWrite(M4_CH, duty);
}

// Cut PWM on all four motors
void motorsStop() {
  ledcWrite(M1_CH, 0);
  ledcWrite(M2_CH, 0);
  ledcWrite(M3_CH, 0);
  ledcWrite(M4_CH, 0);
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

// Configure encoder pins and attach one ISR per motor
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
  
  setupMotors();
  LOG("motors ready\n");

  // startup spin for testing
  LOG("startup spin 1s\n");
  motorM1(true, START_DUTY);
  motorM2(true, START_DUTY);
  motorM3(true, START_DUTY);
  motorM4(true, START_DUTY);
  delay(1000);
  motorsStop();
  LOG("startup spin done\n");
}

// Service OTA, blink LED and log encoder counts once per second
void loop() {
  ArduinoOTA.handle();

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
}