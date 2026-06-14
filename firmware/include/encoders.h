#pragma once
#include <Arduino.h>

extern volatile int32_t encoderCounts[4];

// Configure encoder pins and attach one ISR per motor
void setupEncoders();