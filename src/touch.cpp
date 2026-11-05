#include "touch.h"

#include <Wire.h>

namespace touch {

static const uint8_t kAddr = 0x15;
static const int16_t kSwipeMinPx = 60; // min horizontal travel for a swipe

static bool sDown = false;
static int16_t sStartX, sStartY;
static int16_t sLastX, sLastY;

static void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(kAddr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

static bool readRegs(uint8_t reg, uint8_t *buf, size_t len) {
  Wire.beginTransmission(kAddr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(kAddr, (uint8_t)len) != len) {
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    buf[i] = Wire.read();
  }
  return true;
}

bool begin(uint8_t pinSda, uint8_t pinScl, uint8_t pinRst) {
  pinMode(pinRst, OUTPUT);
  digitalWrite(pinRst, LOW);
  delay(10);
  digitalWrite(pinRst, HIGH);
  delay(100); // controller boot time

  Wire.begin(pinSda, pinScl, 400000);

  uint8_t id = 0;
  if (!readRegs(0xA7, &id, 1)) {
    return false;
  }
  writeReg(0xFE, 0xFF); // disable auto-sleep so polling keeps working
  return true;
}

int readSwipe() {
  // 0x02: finger count, 0x03/0x04: X (12 bit), 0x05/0x06: Y (12 bit)
  uint8_t d[5];
  if (!readRegs(0x02, d, sizeof(d))) {
    return 0;
  }
  const uint8_t fingers = d[0] & 0x0F;
  const int16_t x = ((d[1] & 0x0F) << 8) | d[2];
  const int16_t y = ((d[3] & 0x0F) << 8) | d[4];

  if (fingers > 0) {
    if (!sDown) {
      sDown = true;
      sStartX = x;
      sStartY = y;
    }
    sLastX = x;
    sLastY = y;
    return 0;
  }

  if (!sDown) {
    return 0;
  }
  sDown = false;

  const int16_t dx = sLastX - sStartX;
  const int16_t dy = sLastY - sStartY;
  if (abs(dx) >= kSwipeMinPx && abs(dx) > abs(dy)) {
    return dx < 0 ? 1 : -1; // swipe left -> next, swipe right -> previous
  }
  return 0;
}

} // namespace touch
