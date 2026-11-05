# ESP32RoundTFTProto

Prototype Victorian-style gauges (speed, traction current, voltage) for a model
locomotive, rendered on an OSPTEK 1.51″ round AMOLED.

## Hardware

| Part | Detail |
| ---- | ------ |
| Dev board | [OSPTEK ESP32-S3 Demo Dev Board](https://github.com/osptek/esp32-s3-demo-dev-board) — ESP32-S3R8, 8 MB octal PSRAM, 16 MB flash, CH343P USB-UART, USB-C |
| Display | [OSPTEK 1.51″ 466×466 AMOLED](https://github.com/osptek/display-amoled-1.51inch-466x466) (AM151Q466466LK) — CO5300 driver, QSPI, CST820 touch |
| Adapter | OSPTEK 16-pin AMOLED adapter board |

### Wiring / pin map

The adapter board carries all signals; pins are fixed by the kit
(see [include/pins.h](include/pins.h)):

| Signal | GPIO | | Signal | GPIO |
| ------ | ---- |-| ------ | ---- |
| LCD CS | 14 | | Touch SDA | 41 |
| LCD SCK | 9 | | Touch SCL | 42 |
| LCD D0–D3 | 10–13 | | Touch RST | 40 |
| LCD RST | 15 | | Touch INT | 39 |
| LCD TE | 16 | | | |

## Software stack

- PlatformIO + Arduino framework (ESP32-S3)
- [Arduino_GFX](https://github.com/moononournation/Arduino_GFX) — native CO5300
  QSPI support
- Full-frame canvas in PSRAM, flushed over QSPI (~30 fps)

## Getting started (macOS)

1. **Assemble**: seat the AMOLED's adapter on the dev board's 16-pin headers,
   connect the display FPC, then plug the board into the Mac with a USB-C
   *data* cable.
2. **Check the serial port** — the CH343P bridge is supported natively by
   recent macOS:

   ```sh
   ls /dev/cu.*
   ```

   Expect something like `/dev/cu.wchusbserialXXXX` or `/dev/cu.usbserial-XXXX`.
   If nothing appears, install the
   [WCH CH34x VCP driver](https://github.com/WCHSoftGroup/ch34xser_macos).
3. **Build, flash and monitor** (PlatformIO auto-detects the port):

   ```sh
   pio run                 # build
   pio run -t upload       # flash
   pio device monitor      # serial monitor @ 115200
   ```

   Or use the PlatformIO toolbar in VS Code (✓ build, → upload, 🔌 monitor).
   If upload fails to start, hold **BOOT**, tap **RST**, release **BOOT**,
   and retry.

On boot the board shows the traction-current gauge with a simulated,
easing needle, and logs the simulation targets over serial.

## Project layout

```
├── platformio.ini            # board/build configuration
├── include/
│   ├── pins.h                # kit pin map
│   └── assets/               # generated RGB565 image headers
├── src/
│   ├── main.cpp              # display bring-up + simulation loop
│   └── gauge.{h,cpp}         # reusable gauge renderer (bg + rotated needle sprite)
├── assets/                   # source PNG gauge faces & needle sprites
└── tools/
    ├── png2rgb565.py         # background PNG → C header converter (needs Pillow)
    └── needle2header.py      # needle sprite PNG → C header (colour + alpha)
```

## Adding a gauge face

Export the face as PNG (≤466×466, transparency becomes black), then:

```sh
python3 tools/png2rgb565.py assets/gauge_speed.png gauge_speed_bg include/assets/gauge_speed_bg.h
```

Create a `GaugeConfig` for it in [src/main.cpp](src/main.cpp) — set
`minValue`/`maxValue` to the dial scale and `minAngleDeg`/`maxAngleDeg` to the
needle angles at each end of the scale (degrees, 0° = 3 o'clock,
counter-clockwise positive).

## Adding a needle

Export the needle as an RGBA PNG pointing straight **up**, then convert it,
passing the pivot location in source-image pixels (e.g. the centre of a
466×466 canvas):

```sh
python3 tools/needle2header.py assets/needle_main.png needle_main 232.5 232.5 include/assets/needle_main.h
```

The needle is alpha-blended and rotated about its pivot at render time, so a
single sprite serves every gauge.

## References

- [OSPTEK dev board docs & schematics](https://github.com/osptek/esp32-s3-demo-dev-board/tree/main/docs)
- [Display datasheets & demo code](https://github.com/osptek/display-amoled-1.51inch-466x466)
