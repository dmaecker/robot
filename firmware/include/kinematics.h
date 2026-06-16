#pragma once
#include <Arduino.h>

// Wheel order matches the motor indices: 0=M1 (left-front), 1=M2 (left-rear),
// 2=M3 (right-front), 3=M4 (right-rear). Values are normalized to -100..100.
struct WheelSpeeds { int16_t m[4]; };

// Mecanum mixing.
//   vx    = strafe right (+) / left (-)
//   vy    = forward (+) / backward (-)
//   omega = rotate CCW (+) / CW (-)
//
inline WheelSpeeds mecanumMix(int vx, int vy, int omega) {
  int m1 = vy + vx + omega;  // left-front
  int m2 = vy - vx + omega;  // left-rear
  int m3 = vy - vx - omega;  // right-front
  int m4 = vy + vx - omega;  // right-rear

  int maxMag = max(max(abs(m1), abs(m2)), max(abs(m3), abs(m4)));
  if (maxMag > 100) {
    m1 = m1 * 100 / maxMag;
    m2 = m2 * 100 / maxMag;
    m3 = m3 * 100 / maxMag;
    m4 = m4 * 100 / maxMag;
  }

  return { { (int16_t)m1, (int16_t)m2, (int16_t)m3, (int16_t)m4 } };
}