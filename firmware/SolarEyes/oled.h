#pragma once

#include <stdint.h>

// Starts Wire on PIN_SDA / PIN_SCL at I2C_HZ and sends the SSD1306 init.
void oledBegin();

// Sends a 128x64 page-major framebuffer: 8 pages of 128 bytes, one byte per
// column, bit 0 at the top. This is the layout U8g2's full buffer uses.
void oledPush(const uint8_t* buf);
