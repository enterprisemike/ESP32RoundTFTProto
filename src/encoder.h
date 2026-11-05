// Interrupt-driven quadrature decoder for the onboard EC11 rotary encoder.
#pragma once

#include <Arduino.h>

namespace encoder {

// Attaches interrupts to the A/B (and optional push) pins.
void begin(uint8_t pinA, uint8_t pinB, int8_t pinPush = -1);

// Detent steps accumulated since the last call (+CW / -CCW), then cleared.
int readSteps();

// True once per button press (cleared on read).
bool pressed();

} // namespace encoder
