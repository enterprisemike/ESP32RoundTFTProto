// Minimal CST820 capacitive touch driver with swipe detection.
#pragma once

#include <Arduino.h>

namespace touch {

// Resets and initialises the CST820 over I2C. Returns true if it responds.
bool begin(uint8_t pinSda, uint8_t pinScl, uint8_t pinRst);

// Polls the controller; call once per frame.
// Returns +1 for a swipe left, -1 for a swipe right, 0 otherwise.
int readSwipe();

} // namespace touch
