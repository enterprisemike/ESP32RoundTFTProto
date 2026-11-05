// ESP32RoundTFTProto — Victorian gauge prototype
// OSPTEK ESP32-S3 Demo Dev Board + 1.51" 466x466 AMOLED (CO5300, QSPI)
//
// Three gauge faces (traction current, speed, battery volts), selected with
// the onboard EC11 rotary encoder. Each gauge runs its own needle simulation.

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "assets/gauge_current_bg.h"
#include "assets/gauge_speed_bg.h"
#include "assets/gauge_volts_bg.h"
#include "assets/needle_main.h"
#include "encoder.h"
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

// All dials share the same scale geometry:
// min at lower-left (~200 deg), max at lower-right (~-20 deg).
struct GaugeDef {
  const char *name;
  const char *unit;
  GaugeConfig config;
};

static const GaugeDef kGauges[] = {
    {"TRACTION CURRENT", "A",
     {gauge_current_bg, GAUGE_CURRENT_BG_WIDTH, GAUGE_CURRENT_BG_HEIGHT,
      0.0f, 40.0f, 200.0f, -20.0f, &kMainNeedle}},
    {"SPEED", "km/h",
     {gauge_speed_bg, GAUGE_SPEED_BG_WIDTH, GAUGE_SPEED_BG_HEIGHT,
      0.0f, 15.0f, 200.0f, -20.0f, &kMainNeedle}},
    {"BATTERY VOLTS", "V",
     {gauge_volts_bg, GAUGE_VOLTS_BG_WIDTH, GAUGE_VOLTS_BG_HEIGHT,
      15.0f, 25.0f, 200.0f, -20.0f, &kMainNeedle,
      20.0f, RGB565(255, 40, 40), 60}},
};
static const int kGaugeCount = sizeof(kGauges) / sizeof(kGauges[0]);

static Gauge gauges[kGaugeCount] = {
    Gauge(kGauges[0].config), Gauge(kGauges[1].config), Gauge(kGauges[2].config)};

// Per-gauge simulation: the needle eases toward a wandering target.
struct Sim {
  float value;
  float target;
  uint32_t lastTargetMs;
};
static Sim sims[kGaugeCount];

static int selected = 0;

static float randomInRange(float lo, float hi) {
  return lo + (hi - lo) * (random(0, 1000) / 1000.0f);
}

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

  encoder::begin(PIN_ENC_A, PIN_ENC_B, PIN_ENC_PUSH);
  Serial.println("Encoder ready — rotate to switch gauges");

  for (int i = 0; i < kGaugeCount; i++) {
    const GaugeConfig &c = kGauges[i].config;
    sims[i] = {c.minValue, randomInRange(c.minValue, c.maxValue), 0};
  }
  Serial.printf("Gauge: %s\n", kGauges[selected].name);
}

void loop() {
  const uint32_t now = millis();

  // Encoder: rotate to switch gauges (wraps around).
  const int steps = encoder::readSteps();
  if (steps != 0) {
    selected = ((selected + steps) % kGaugeCount + kGaugeCount) % kGaugeCount;
    Serial.printf("Gauge: %s\n", kGauges[selected].name);
  }
  if (encoder::pressed()) {
    Serial.println("Encoder pressed (unassigned)");
  }

  // Advance only the visible gauge's simulation.
  Sim &sim = sims[selected];
  const GaugeConfig &cfg = kGauges[selected].config;
  if (now - sim.lastTargetMs > 3000) {
    sim.lastTargetMs = now;
    sim.target = randomInRange(cfg.minValue, cfg.maxValue);
    Serial.printf("Sim target: %.1f %s\n", sim.target, kGauges[selected].unit);
  }

  // Ease toward the target with a little jitter, like a real mechanical needle.
  sim.value += (sim.target - sim.value) * 0.06f;
  const float span = cfg.maxValue - cfg.minValue;
  const float displayed = sim.value + span * (random(-10, 10) / 10000.0f);

  gauges[selected].draw(canvas, displayed);
  canvas->flush();

  delay(33); // ~30 fps
}
