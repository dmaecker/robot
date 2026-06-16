#pragma once
#include <Arduino.h>

// Start UDP comm channel to the Raspberry Pi bridge.
void setupComm();

// Service incoming cmd packets + failsafe
void commPoll();

// Push telemetry (encoder counts + timestamp) to the Pi
void commSendTelemetry();
