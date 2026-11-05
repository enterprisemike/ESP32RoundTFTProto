// Victorian-style gauge rendering over a bitmap background.
#pragma once

#include <Arduino_GFX_Library.h>

// Needle artwork authored pointing straight UP, with the rotation pivot
// given in sprite pixel coordinates.
struct NeedleSprite {
  const uint16_t *color; // RGB565 (PROGMEM)
  const uint8_t *alpha;  // 0-255 coverage (PROGMEM)
  int16_t width;
  int16_t height;
  float pivotX;
  float pivotY;
};

struct GaugeConfig {
  const uint16_t *background; // RGB565 background bitmap (PROGMEM)
  int16_t bgWidth;
  int16_t bgHeight;
  float minValue;      // value at minAngle
  float maxValue;      // value at maxAngle
  float minAngleDeg;   // needle angle at minValue (math convention: 0 = east, CCW+)
  float maxAngleDeg;   // needle angle at maxValue
  const NeedleSprite *needle;
};

class Gauge {
public:
  explicit Gauge(const GaugeConfig &cfg) : _cfg(cfg) {}

  // Draws the background and needle into the canvas (full redraw).
  void draw(Arduino_Canvas *canvas, float value) const;

private:
  GaugeConfig _cfg;
};
