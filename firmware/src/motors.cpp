#include "motors.h"
#include "config.h"

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
