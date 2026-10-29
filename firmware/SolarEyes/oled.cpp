#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "oled.h"

static void oledCmd(uint8_t value) {
  Wire.beginTransmission(OLED_ADDR);
  Wire.write(0x00);  // control byte: command
  Wire.write(value);
  Wire.endTransmission();
}

void oledBegin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(I2C_HZ);
  Wire.setTimeOut(I2C_TIMEOUT_MS);
  const uint8_t seq[] = {
      0xAE,        // display off
      0xD5, 0x80,  // clock divide
      0xA8, 0x3F,  // multiplex, 64 rows
      0xD3, 0x00,  // display offset
      0x40,        // start line 0
      0x8D, 0x14,  // charge pump on
      0x20, 0x00,  // horizontal addressing
      0xA1,        // segment remap
      0xC8,        // COM scan from the bottom
      0xDA, 0x12,  // COM pins
      0x81, 0xCF,  // contrast
      0xD9, 0xF1,  // pre-charge
      0xDB, 0x40,  // VCOMH
      0xA4,        // show RAM
      0xA6,        // normal, not inverted
      0xAF};       // display on
  for (uint8_t value : seq) {
    oledCmd(value);
  }
}

void oledPush(const uint8_t* buf) {
  for (uint8_t page = 0; page < SCREEN_H / 8; page++) {
    oledCmd(0xB0 | page);
    oledCmd(0x00 | (OLED_COL_OFFSET & 0x0F));
    oledCmd(0x10 | (OLED_COL_OFFSET >> 4));
    const uint8_t* row = buf + (page * SCREEN_W);
    // 16 data bytes per transaction keeps each write well under the Wire
    // buffer.
    for (int col = 0; col < SCREEN_W; col += 16) {
      Wire.beginTransmission(OLED_ADDR);
      Wire.write(0x40);  // control byte: data
      for (int i = 0; i < 16; i++) {
        Wire.write(row[col + i]);
      }
      Wire.endTransmission();
    }
  }
}
