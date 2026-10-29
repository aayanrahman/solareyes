# Working on Solar Eyes

The animation can be changed and checked on a computer, then flashed to the SuperMini over USB-C. Flip the keychain's switch off before plugging in.

## Where to edit

| Change | File |
| --- | --- |
| Pins, OLED address, I2C clock, eye positions, frame rate | `firmware/SolarEyes/config.h` |
| Which expression comes next, how long it lasts, how often | `firmware/SolarEyes/eyes.cpp` (`ROTATION`) |
| The shape of each expression | `firmware/SolarEyes/eyes.cpp` (`eyesTarget`) |
| How eyes and brows are drawn | `firmware/SolarEyes/render.cpp` |
| SSD1306 init, pushing the frame over I2C | `firmware/SolarEyes/oled.cpp` |
| Boot and the frame loop | `firmware/SolarEyes/SolarEyes.ino` |

`eyes.cpp` has no Arduino or U8g2 code in it, so it builds and runs on a laptop. Keep it that way: anything that touches `Wire`, `u8g2`, or `esp_random()` belongs in the other files.

The weights in `ROTATION` add up to 100. A new expression needs a row there, a case in `eyesTarget`, and a check in `test/test_eyes.cpp`.

## Checks

```bash
make test
```

That compiles `firmware/SolarEyes/eyes.cpp` with the tests in `test/` and checks the expression odds and timing, the easing, the idle bob, and that every pose stays on the blue part of the panel.

```bash
make firmware
```

That needs [arduino-cli](https://arduino.github.io/arduino-cli/), the `esp32:esp32` core, and the U8g2 library. It builds the sketch for the ESP32C3 Dev Module with USB CDC On Boot enabled.

```bash
pio run -e supermini -t upload
pio device monitor -b 115200
```

PlatformIO fetches U8g2 on its own.

## Notes from the bench

- The panel only answered at 100 kHz. Leave `I2C_HZ` there unless a different module is fitted.
- The top 16 rows are yellow glass. `BLUE_TOP` has to stay a multiple of 8, because the sketch blanks whole OLED pages.
- U8g2 only draws into RAM. `u8g2.begin()` is never called, and `oled.cpp` sends the buffer itself.
- The C3 is single-core. `loop()` sleeps between frames so USB serial stays up.
- Serial output is 115200 baud.
