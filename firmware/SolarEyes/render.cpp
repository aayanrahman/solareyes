#include <Arduino.h>
#include <U8g2lib.h>
#include <string.h>

#include "config.h"
#include "oled.h"
#include "render.h"

// U8g2 only draws into its 1 KB buffer here. u8g2.begin() is never called;
// oled.cpp sends the buffer itself at the 100 kHz this panel needs. The
// constructor sets the buffer layout, so the pins passed to it go unused.
// If the yellow band sits at the bottom after mounting, use U8G2_R2 and
// revisit BLUE_TOP in config.h.
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0, U8X8_PIN_NONE, /* clock */ PIN_SCL, /* data */ PIN_SDA);

static void drawEye(int cx, int cy, int w, int h) {
  EyeBox box = eyeBox(cx, cy, w, h);
  u8g2.setDrawColor(1);
  u8g2.drawRBox(box.x, box.y, box.w, box.h, box.r);
}

static void drawBrow(int cx, int y, int w, int arch) {
  int x0 = cx - w / 2;
  int x1 = cx + w / 2;
  int yEnd = y;
  int yMid = y - arch;
  if (yEnd < BLUE_TOP) {
    yEnd = BLUE_TOP;
  }
  if (yMid < BLUE_TOP) {
    yMid = BLUE_TOP;
  }
  u8g2.setDrawColor(1);
  for (int t = 0; t < 2; t++) {
    u8g2.drawLine(x0, yEnd + t, cx, yMid + t);
    u8g2.drawLine(cx, yMid + t, x1, yEnd + t);
  }
}

static void keepBlueOnly() {
  // BLUE_TOP rows are BLUE_TOP / 8 whole pages at the start of the buffer.
  memset(u8g2.getBufferPtr(), 0, SCREEN_W * (BLUE_TOP / 8));
}

void renderFrame(const Eyes& eyes, uint32_t nowMs) {
  const Pose& shown = eyes.shown;
  int bobL = eyesBob(nowMs);
  int bobR = eyesBob(nowMs + 80);
  int browYL = BROW_Y + shown.browL + bobL;
  int browYR = BROW_Y + shown.browR + bobR;
  if (browYL < BLUE_TOP) {
    browYL = BLUE_TOP;
  }
  if (browYR < BLUE_TOP) {
    browYR = BLUE_TOP;
  }

  u8g2.clearBuffer();
  drawBrow(LEFT_X + shown.dxL, browYL, shown.wL, shown.archL);
  drawBrow(RIGHT_X + shown.dxR, browYR, shown.wR, shown.archR);
  drawEye(LEFT_X + shown.dxL, EYE_Y + shown.dyL + bobL, shown.wL, shown.hL);
  drawEye(RIGHT_X + shown.dxR, EYE_Y + shown.dyR + bobR, shown.wR, shown.hR);
  keepBlueOnly();
  oledPush(u8g2.getBufferPtr());
}
