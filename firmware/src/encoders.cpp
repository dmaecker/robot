#include "encoders.h"
#include "config.h"

volatile int32_t encoderCounts[4] = {0, 0, 0, 0};

// Per-motor encoder ISR; counts up/down from A==B on channel-A edge
template <uint8_t IDX>
void IRAM_ATTR onEncoderA() {
  bool a = digitalRead(ENCODERS[IDX].a);
  bool b = digitalRead(ENCODERS[IDX].b);
  encoderCounts[IDX] += (a == b) ? 1 : -1;
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
