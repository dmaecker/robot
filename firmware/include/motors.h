#pragma once
#include <Arduino.h>

// Init direction pins and PWM channels, leave all motors stopped
void setupMotors();

// Drive left-front motor: direction + duty
void motorM1(bool fwd, uint8_t duty);

// Drive left-rear motor: direction + duty
void motorM2(bool fwd, uint8_t duty);

// Drive right-front motor; polarity inverted (right side mirrored)
void motorM3(bool fwd, uint8_t duty);

// Drive right-rear motor; polarity inverted (right side mirrored)
void motorM4(bool fwd, uint8_t duty);

// Cut PWM on all four motors
void motorsStop();