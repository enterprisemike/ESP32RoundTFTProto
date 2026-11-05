#include "gauge.h"

#include "pins.h"

// Alpha-blend fg over bg in RGB565.
static inline uint16_t blend565(uint16_t fg, uint16_t bg, uint8_t a) {
  const uint32_t ia = 255 - a;
  const uint32_t r = (((fg >> 11) & 0x1F) * a + ((bg >> 11) & 0x1F) * ia) / 255;
  const uint32_t g = (((fg >> 5) & 0x3F) * a + ((bg >> 5) & 0x3F) * ia) / 255;
  const uint32_t b = ((fg & 0x1F) * a + (bg & 0x1F) * ia) / 255;
  return (r << 11) | (g << 5) | b;
}

// Blit the needle sprite rotated about its pivot, placed at (destX, destY) in
// the canvas framebuffer. needleAngleRad is the needle direction in math
// convention (0 = east, CCW positive); the sprite is authored pointing up.
static void blitNeedle(Arduino_Canvas *canvas, const NeedleSprite &spr,
                       float destX, float destY, float needleAngleRad) {
  uint16_t *fb = canvas->getFramebuffer();
  const int16_t fbW = canvas->width();
  const int16_t fbH = canvas->height();

  // Sprite-to-screen rotation: beta = 90 deg - needle angle.
  const float beta = HALF_PI - needleAngleRad;
  const float c = cosf(beta);
  const float s = sinf(beta);

  // Bounding box: circle around the pivot reaching the farthest sprite corner.
  const float rx = max(spr.pivotX, spr.width - spr.pivotX);
  const float ry = max(spr.pivotY, spr.height - spr.pivotY);
  const int16_t r = (int16_t)ceilf(sqrtf(rx * rx + ry * ry)) + 1;
  const int16_t x0 = max((int16_t)0, (int16_t)(destX - r));
  const int16_t x1 = min((int16_t)(fbW - 1), (int16_t)(destX + r));
  const int16_t y0 = max((int16_t)0, (int16_t)(destY - r));
  const int16_t y1 = min((int16_t)(fbH - 1), (int16_t)(destY + r));

  // Inverse mapping, fixed point 16.16, stepped incrementally per pixel:
  // src = R(-beta) * (dest - pivotDest) + pivotSrc
  const int32_t cf = (int32_t)lroundf(c * 65536.0f);
  const int32_t sf = (int32_t)lroundf(s * 65536.0f);

  for (int16_t y = y0; y <= y1; y++) {
    const float dy = (y + 0.5f) - destY;
    const float dx = (x0 + 0.5f) - destX;
    int32_t sx = (int32_t)lroundf((c * dx + s * dy + spr.pivotX) * 65536.0f);
    int32_t sy = (int32_t)lroundf((-s * dx + c * dy + spr.pivotY) * 65536.0f);
    uint16_t *row = fb + (int32_t)y * fbW;

    for (int16_t x = x0; x <= x1; x++, sx += cf, sy -= sf) {
      const int32_t ix = sx >> 16;
      const int32_t iy = sy >> 16;
      if ((uint32_t)ix < (uint32_t)spr.width && (uint32_t)iy < (uint32_t)spr.height) {
        const int32_t idx = iy * spr.width + ix;
        const uint8_t a = spr.alpha[idx];
        if (a == 255) {
          row[x] = spr.color[idx];
        } else if (a) {
          row[x] = blend565(spr.color[idx], row[x], a);
        }
      }
    }
  }
}

void Gauge::draw(Arduino_Canvas *canvas, float value) const {
  // Pick the normal or low-warning background.
  const uint16_t *bg = _cfg.background;
  if (_cfg.lowTintAlpha != 0 && value < _cfg.lowThreshold) {
    if (_lowBg == nullptr) {
      const size_t n = (size_t)_cfg.bgWidth * _cfg.bgHeight;
      _lowBg = (uint16_t *)ps_malloc(n * sizeof(uint16_t));
      if (_lowBg != nullptr) {
        for (size_t i = 0; i < n; i++) {
          const uint16_t p = _cfg.background[i];
          // Leave the pure-black surround untouched.
          _lowBg[i] = (p == 0) ? 0 : blend565(_cfg.lowTintColor, p, _cfg.lowTintAlpha);
        }
      }
    }
    if (_lowBg != nullptr) {
      bg = _lowBg;
    }
  }

  // Centre the background on the round panel.
  const int16_t bgX = (LCD_WIDTH - _cfg.bgWidth) / 2;
  const int16_t bgY = (LCD_HEIGHT - _cfg.bgHeight) / 2;
  canvas->fillScreen(RGB565_BLACK);
  canvas->draw16bitRGBBitmap(bgX, bgY, const_cast<uint16_t *>(bg),
                             _cfg.bgWidth, _cfg.bgHeight);

  // Map value to needle angle.
  const float clamped =
      constrain(value, min(_cfg.minValue, _cfg.maxValue), max(_cfg.minValue, _cfg.maxValue));
  const float t = (clamped - _cfg.minValue) / (_cfg.maxValue - _cfg.minValue);
  const float angleRad =
      (_cfg.minAngleDeg + t * (_cfg.maxAngleDeg - _cfg.minAngleDeg)) * DEG_TO_RAD;

  blitNeedle(canvas, *_cfg.needle, LCD_WIDTH / 2.0f, LCD_HEIGHT / 2.0f, angleRad);
}
