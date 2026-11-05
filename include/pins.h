// Pin map: OSPTEK ESP32-S3 Demo Dev Board + 1.51" AMOLED adapter
// Source: osptek/display-amoled-1.51inch-466x466 example
// (esp32s3-idf5-co5300-qspi-lvgl-adapter-lvgl8)
#pragma once

// CO5300 AMOLED over QSPI
#define PIN_LCD_CS 14
#define PIN_LCD_SCK 9
#define PIN_LCD_D0 10
#define PIN_LCD_D1 11
#define PIN_LCD_D2 12
#define PIN_LCD_D3 13
#define PIN_LCD_RST 15
#define PIN_LCD_TE 16 // tearing effect, unused for now

// CST820 capacitive touch (I2C)
#define PIN_TOUCH_SDA 41
#define PIN_TOUCH_SCL 42
#define PIN_TOUCH_RST 40
#define PIN_TOUCH_INT 39

// Panel geometry
#define LCD_WIDTH 466
#define LCD_HEIGHT 466
#define LCD_COL_OFFSET 6 // CO5300 x-gap per vendor example
#define LCD_ROW_OFFSET 0
