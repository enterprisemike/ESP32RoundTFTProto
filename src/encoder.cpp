#include "encoder.h"

namespace encoder {

static uint8_t sPinA, sPinB;
static volatile uint8_t sState = 0;       // previous + current AB levels
static volatile int32_t sQuarterSteps = 0;
static volatile int32_t sDetents = 0;
static volatile bool sPressed = false;
static volatile uint32_t sLastPressMs = 0;

// Quadrature transition table indexed by [prevAB:curAB].
static const int8_t kTransition[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0};

static void IRAM_ATTR onEdge() {
  const uint8_t ab = (digitalRead(sPinA) << 1) | digitalRead(sPinB);
  sState = ((sState << 2) | ab) & 0x0F;
  sQuarterSteps += kTransition[sState];
  // This EC11 variant produces one half quadrature cycle (2 quarter-steps)
  // per detent.
  if (sQuarterSteps >= 2) {
    sDetents++;
    sQuarterSteps = 0;
  } else if (sQuarterSteps <= -2) {
    sDetents--;
    sQuarterSteps = 0;
  }
}

static void IRAM_ATTR onPress() {
  const uint32_t now = millis();
  if (now - sLastPressMs > 250) { // debounce
    sPressed = true;
    sLastPressMs = now;
  }
}

void begin(uint8_t pinA, uint8_t pinB, int8_t pinPush) {
  sPinA = pinA;
  sPinB = pinB;
  pinMode(pinA, INPUT_PULLUP); // board also has external 10K pullups
  pinMode(pinB, INPUT_PULLUP);
  sState = (digitalRead(pinA) << 1) | digitalRead(pinB);
  attachInterrupt(digitalPinToInterrupt(pinA), onEdge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinB), onEdge, CHANGE);
  if (pinPush >= 0) {
    pinMode(pinPush, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pinPush), onPress, FALLING);
  }
}

int readSteps() {
  noInterrupts();
  const int32_t steps = sDetents;
  sDetents = 0;
  interrupts();
  return steps;
}

bool pressed() {
  noInterrupts();
  const bool p = sPressed;
  sPressed = false;
  interrupts();
  return p;
}

} // namespace encoder
