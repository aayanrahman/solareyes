#pragma once

#include <stdint.h>

// SDA = GPIO6, SCL = GPIO7. These are chosen pins for this build.
// They are not a universal ESP32-C3 default, so oledBegin() passes them to
// Wire explicitly.
constexpr uint8_t PIN_SDA = 6;
constexpr uint8_t PIN_SCL = 7;

// SSD1306 on the 0.96" module.
constexpr uint8_t OLED_ADDR = 0x3C;

// This panel did not respond at the library's default 400 kHz.
constexpr uint32_t I2C_HZ = 100000;
constexpr uint16_t I2C_TIMEOUT_MS = 50;

// First RAM column of the panel. SSD1306 starts at 0. If the picture is
// shifted two pixels sideways or has a stripe of noise on one edge, the
// controller is likely an SH1106 (132-column RAM); set this to 2.
constexpr uint8_t OLED_COL_OFFSET = 0;

constexpr int SCREEN_W = 128;
constexpr int SCREEN_H = 64;

// Rows 0-15 are the yellow glass. Leaving them off keeps the lit pixels blue.
// This assumes the module is upright (U8G2_R0 in render.cpp).
constexpr int BLUE_TOP = 16;

// Resting centers of the two eyes, and the brow line above them.
constexpr int LEFT_X = 34;
constexpr int RIGHT_X = 96;
constexpr int EYE_Y = 44;
constexpr int BROW_Y = 18;

// Smallest eye that still reads as an eye, and the corner radius at rest.
constexpr int EYE_MIN_W = 10;
constexpr int EYE_MIN_H = 3;
constexpr int EYE_RADIUS = 5;

constexpr uint32_t FRAME_MS = 33;

// C3 is single-core. A tight spin between frames starves USB and the port
// drops, so loop() sleeps this long while it waits.
constexpr uint32_t IDLE_DELAY_MS = 5;

// How long the eyes sit still after power-on before the first expression.
constexpr uint32_t BOOT_IDLE_MS = 1600;

constexpr uint32_t SERIAL_BAUD = 115200;

// Prints how long each frame takes to push over I2C, once a second. The
// frame cannot run faster than the push.
#define DEBUG_FRAME_TIME 0
