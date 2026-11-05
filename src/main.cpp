// ESP32RoundTFTProto — Victorian gauge prototype
// OSPTEK ESP32-S3 Demo Dev Board + 1.51" 466x466 AMOLED (CO5300, QSPI)
//
// Displays the traction-current gauge background and sweeps a simulated needle.

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "assets/gauge_current_bg.h"
#include "assets/needle_main.h"
#include "gauge.h"
#include "pins.h"

static Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    PIN_LCD_CS, PIN_LCD_SCK, PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3);

static Arduino_CO5300 *panel = new Arduino_CO5300(
    bus, PIN_LCD_RST, 0 /* rotation */, LCD_WIDTH, LCD_HEIGHT,
    LCD_COL_OFFSET, LCD_ROW_OFFSET, 0, 0);

// Full-frame canvas in PSRAM to avoid flicker; flushed per frame over QSPI.
static Arduino_Canvas *canvas =
    new Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, panel);

// Victorian clock-hand needle, authored pointing up, pivot on the brass collet.
static const NeedleSprite kMainNeedle = {
    .color = needle_main,
    .alpha = needle_main_alpha,
    .width = NEEDLE_MAIN_WIDTH,
    .height = NEEDLE_MAIN_HEIGHT,
    .pivotX = NEEDLE_MAIN_PIVOT_X,
    .pivotY = NEEDLE_MAIN_PIVOT_Y,
};

// Traction current: 0 A at lower-left (~200 deg), 40 A at lower-right (~-20 deg).
static const GaugeConfig kCurrentGauge = {
    .background = gauge_current_bg,
    .bgWidth = GAUGE_CURRENT_BG_WIDTH,
    .bgHeight = GAUGE_CURRENT_BG_HEIGHT,
    .minValue = 0.0f,
    .maxValue = 40.0f,
    .minAngleDeg = 200.0f,
    .maxAngleDeg = -20.0f,
    .needle = &kMainNeedle,
};

static Gauge currentGauge(kCurrentGauge);

// Simple simulation: needle eases toward a wandering target.
static float simValue = 0.0f;
static float simTarget = 25.0f;
static uint32_t lastTargetMs = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nESP32RoundTFTProto — gauge prototype starting");
  Serial.printf("PSRAM: %u bytes free\n", ESP.getFreePsram());

  if (!canvas->begin(40000000L /* 40 MHz QSPI */)) {
    Serial.println("ERROR: display init failed!");
    while (true) {
      delay(1000);
    }
  }
  panel->setBrightness(200);
  Serial.println("Display initialised (CO5300 466x466 QSPI)");
}

void loop() {
  const uint32_t now = millis();

  // Pick a new simulated load every few seconds.
  if (now - lastTargetMs > 3000) {
    lastTargetMs = now;
    simTarget = random(0, 400) / 10.0f;
    Serial.printf("Sim target: %.1f A\n", simTarget);
  }

  // Ease toward the target with a little jitter, like a real mechanical needle.
  simValue += (simTarget - simValue) * 0.06f;
  const float displayed = simValue + random(-10, 10) / 60.0f;

  currentGauge.draw(canvas, displayed);
  canvas->flush();

  delay(33); // ~30 fps
}
