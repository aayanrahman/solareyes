#pragma once

#include <stdint.h>

#include "config.h"

// Happy has a pose but is not in the random rotation yet.
enum class Expr : uint8_t {
  Idle,
  Blink,
  LookSide,
  LookUp,
  LookDown,
  Happy,
  Wink,
  Sleepy
};

// Offsets and sizes for both eyes and brows, in pixels. L is the eye on the
// left of the screen.
struct Pose {
  int dxL;
  int dxR;
  int dyL;
  int dyR;
  int wL;
  int wR;
  int hL;
  int hR;
  int browL;
  int browR;
  int archL;
  int archR;
};

struct Eyes {
  Expr expr;
  int8_t glanceDir;  // -1 looks left, 1 looks right
  bool winkLeft;
  uint32_t exprEnd;
  Pose shown;  // what is on the panel now; eases toward eyesTarget()
};

// A filled rounded rectangle, already clamped so U8g2 can draw it.
struct EyeBox {
  int x;
  int y;
  int w;
  int h;
  int r;
};

typedef uint32_t (*RandomFn)();

extern const Pose REST_POSE;

void eyesInit(Eyes& eyes, uint32_t nowMs);
void eyesEnter(Eyes& eyes, Expr next, uint32_t nowMs, uint32_t durationMs);

// Picks the next expression once the current one has run its time. Safe
// across millis() wraparound. Returns true when a new expression started.
bool eyesUpdate(Eyes& eyes, uint32_t nowMs, RandomFn random);

Pose eyesTarget(const Eyes& eyes);

// One frame of easing from eyes.shown toward eyesTarget().
void eyesStep(Eyes& eyes);

// Idle bob, -2 to 2 pixels.
int eyesBob(uint32_t nowMs);

void approach(int& value, int target, int step);

EyeBox eyeBox(int cx, int cy, int w, int h);
