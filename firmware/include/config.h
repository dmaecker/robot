#pragma once
#include <Arduino.h>

// === General ===
inline constexpr char     HOSTNAME[] = "robot_control";
inline constexpr uint8_t  RGB_PIN    = 38;  // onboard RGB LED

// === PWM (LEDC) ===
inline constexpr int     PWM_FREQ   = 20000;
inline constexpr int     PWM_RES    = 8;     // 8-bit duty (0..255)
inline constexpr uint8_t START_DUTY = 80;    // startup test duty

// LEDC channels (one per motor)
inline constexpr int M1_CH = 0;
inline constexpr int M2_CH = 1;
inline constexpr int M3_CH = 2;
inline constexpr int M4_CH = 3;

// === Left driver (M1 + M2) ===
inline constexpr uint8_t L_PWMA = 4;   // M1
inline constexpr uint8_t L_AIN1 = 6;
inline constexpr uint8_t L_AIN2 = 5;
inline constexpr uint8_t L_PWMB = 16;  // M2
inline constexpr uint8_t L_BIN1 = 7;
inline constexpr uint8_t L_BIN2 = 15;

// === Right driver (M3 + M4) ===
inline constexpr uint8_t R_PWMA = 40;  // M3
inline constexpr uint8_t R_AIN1 = 41;
inline constexpr uint8_t R_AIN2 = 39;
inline constexpr uint8_t R_PWMB = 1;   // M4
inline constexpr uint8_t R_BIN1 = 42;
inline constexpr uint8_t R_BIN2 = 2;

// === Encoders ===
struct EncoderPins { uint8_t a; uint8_t b; const char* name; };
inline constexpr EncoderPins ENCODERS[4] = {
  {17, 18, "M1"},
  { 8,  9, "M2"},
  {48, 47, "M3"},
  {21, 14, "M4"},
};