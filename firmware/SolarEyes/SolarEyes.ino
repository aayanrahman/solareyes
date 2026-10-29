// Solar Eyes v0.1
// ESP32-C3 SuperMini + 0.96" 128x64 I2C OLED.
// Open this folder in the Arduino IDE. Wiring is in the README.
// Pins, the I2C clock, and the eye layout live in config.h.

#include <Arduino.h>

#include "config.h"
#include "eyes.h"
#include "oled.h"
#include "render.h"

static_assert(BLUE_TOP % 8 == 0, "yellow band must be whole OLED pages");

static Eyes eyes;
static uint32_t nextFrame = 0;

void setup() {
  Serial.begin(SERIAL_BAUD);

  // Same 100 kHz init that lit this panel. U8g2 only draws into RAM.
  oledBegin();

  uint32_t now = millis();
  eyesInit(eyes, now);
  nextFrame = now;
  renderFrame(eyes, now);

  Serial.println("Solar Eyes v0.1");
  Serial.println("SDA GPIO6, SCL GPIO7, SSD1306 128x64 @ 100 kHz");
}

void loop() {
  uint32_t now = millis();

  // Wi-Fi and BLE are never started. esp_random() still returns hardware
  // noise from the RNG peripheral, which is plenty for picking expressions.
  eyesUpdate(eyes, now, esp_random);

  if ((int32_t)(now - nextFrame) < 0) {
    delay(IDLE_DELAY_MS);
    return;
  }
  nextFrame = now + FRAME_MS;
  eyesStep(eyes);
#if DEBUG_FRAME_TIME
  uint32_t start = micros();
  renderFrame(eyes, now);
  static uint32_t lastReport = 0;
  if (now - lastReport >= 1000) {
    lastReport = now;
    Serial.print("frame us: ");
    Serial.println(micros() - start);
  }
#else
  renderFrame(eyes, now);
#endif
}
