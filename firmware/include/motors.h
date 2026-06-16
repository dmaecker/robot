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

// Drive a single motor by signed speed (-255..255); sign sets direction.
// Index: 0=M1, 1=M2, 2=M3, 3=M4.
void setMotor(uint8_t idx, int16_t speed);

// Mix a drive command (vx strafe, vy forward, omega rotate; each ~ -100..100)
// through the mecanum kinematics and apply it to all four wheels.
void driveRobot(int vx, int vy, int omega);
