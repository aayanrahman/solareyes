#include "eyes.h"

const Pose REST_POSE = {0, 0, 0, 0, 26, 26, 34, 34, 0, 0, 4, 4};

struct Slot {
  Expr expr;
  uint8_t weight;  // out of 100
  uint16_t baseMs;
  uint16_t jitterMs;  // extra time, picked at random from 0 to jitterMs - 1
};

static const Slot ROTATION[] = {
    {Expr::Idle, 18, 420, 280},
    {Expr::Blink, 16, 170, 0},
    {Expr::LookSide, 24, 480, 280},
    {Expr::LookUp, 14, 420, 0},
    {Expr::LookDown, 12, 420, 0},
    {Expr::Wink, 9, 260, 0},
    {Expr::Sleepy, 7, 700, 0},
};

void eyesInit(Eyes& eyes, uint32_t nowMs) {
  eyes.glanceDir = 1;
  eyes.winkLeft = true;
  eyes.shown = REST_POSE;
  eyesEnter(eyes, Expr::Idle, nowMs, BOOT_IDLE_MS);
}

void eyesEnter(Eyes& eyes, Expr next, uint32_t nowMs, uint32_t durationMs) {
  eyes.expr = next;
  eyes.exprEnd = nowMs + durationMs;
}

bool eyesUpdate(Eyes& eyes, uint32_t nowMs, RandomFn random) {
  if ((int32_t)(nowMs - eyes.exprEnd) < 0) {
    return false;
  }

  uint32_t roll = random() % 100;
  const Slot* slot = &ROTATION[0];
  for (const Slot& candidate : ROTATION) {
    slot = &candidate;
    if (roll < candidate.weight) {
      break;
    }
    roll -= candidate.weight;
  }

  uint32_t duration = slot->baseMs;
  if (slot->jitterMs > 0) {
    duration += random() % slot->jitterMs;
  }
  if (slot->expr == Expr::LookSide) {
    eyes.glanceDir = (random() & 1) ? 1 : -1;
  } else if (slot->expr == Expr::Wink) {
    eyes.winkLeft = random() & 1;
  }
  eyesEnter(eyes, slot->expr, nowMs, duration);
  return true;
}

void approach(int& value, int target, int step) {
  if (value < target) {
    int next = value + step;
    value = next > target ? target : next;
  } else if (value > target) {
    int next = value - step;
    value = next < target ? target : next;
  }
}

Pose eyesTarget(const Eyes& eyes) {
  Pose pose = REST_POSE;
  switch (eyes.expr) {
    case Expr::Idle:
      break;
    case Expr::Blink:
      pose.hL = 4;
      pose.hR = 4;
      pose.browL = 2;
      pose.browR = 2;
      pose.archL = 2;
      pose.archR = 2;
      break;
    case Expr::LookSide:
      // The near eye grows, the far eye shrinks, and both slide together.
      if (eyes.glanceDir < 0) {
        pose.dxL = -2;
        pose.dxR = -8;
        pose.wL = 28;
        pose.wR = 23;
        pose.hL = 36;
        pose.hR = 31;
        pose.browL = -1;
        pose.browR = 0;
        pose.archL = 5;
        pose.archR = 3;
      } else {
        pose.dxL = 8;
        pose.dxR = 2;
        pose.wL = 23;
        pose.wR = 28;
        pose.hL = 31;
        pose.hR = 36;
        pose.browL = 0;
        pose.browR = -1;
        pose.archL = 3;
        pose.archR = 5;
      }
      break;
    case Expr::LookUp:
      pose.dyL = -2;
      pose.dyR = -2;
      pose.hL = 36;
      pose.hR = 36;
      pose.browL = -2;
      pose.browR = -2;
      pose.archL = 5;
      pose.archR = 5;
      break;
    case Expr::LookDown:
      pose.dyL = 3;
      pose.dyR = 3;
      pose.hL = 26;
      pose.hR = 26;
      pose.browL = 2;
      pose.browR = 2;
      pose.archL = 2;
      pose.archR = 2;
      break;
    case Expr::Happy:
      pose.wL = 28;
      pose.wR = 28;
      pose.hL = 32;
      pose.hR = 32;
      pose.browL = -2;
      pose.browR = -2;
      pose.archL = 5;
      pose.archR = 5;
      break;
    case Expr::Wink:
      if (eyes.winkLeft) {
        pose.dxL = 2;
        pose.hL = 4;
        pose.browL = 1;
        pose.archL = 2;
      } else {
        pose.dxR = -2;
        pose.hR = 4;
        pose.browR = 1;
        pose.archR = 2;
      }
      break;
    case Expr::Sleepy:
      pose.dyL = 2;
      pose.dyR = 2;
      pose.hL = 22;
      pose.hR = 22;
      pose.browL = 2;
      pose.browR = 2;
      pose.archL = 2;
      pose.archR = 2;
      break;
  }
  return pose;
}

void eyesStep(Eyes& eyes) {
  Pose goal = eyesTarget(eyes);
  Pose& shown = eyes.shown;
  approach(shown.dxL, goal.dxL, 1);
  approach(shown.dxR, goal.dxR, 1);
  approach(shown.dyL, goal.dyL, 1);
  approach(shown.dyR, goal.dyR, 1);
  approach(shown.wL, goal.wL, 1);
  approach(shown.wR, goal.wR, 1);
  approach(shown.hL, goal.hL, 2);
  approach(shown.hR, goal.hR, 2);
  approach(shown.browL, goal.browL, 1);
  approach(shown.browR, goal.browR, 1);
  approach(shown.archL, goal.archL, 1);
  approach(shown.archR, goal.archR, 1);
}

int eyesBob(uint32_t nowMs) {
  int bob = (int)((nowMs / 140) % 8);
  if (bob > 4) {
    bob = 8 - bob;
  }
  return bob - 2;
}

EyeBox eyeBox(int cx, int cy, int w, int h) {
  if (w < EYE_MIN_W) {
    w = EYE_MIN_W;
  }
  if (h < EYE_MIN_H) {
    h = EYE_MIN_H;
  }
  int y = cy - h / 2;
  if (y < BLUE_TOP) {
    h -= BLUE_TOP - y;
    y = BLUE_TOP;
  }
  if (h < 1) {
    h = 1;
  }
  // U8g2 drawRBox() needs w >= 2 * (r + 1) and h >= 2 * (r + 1).
  int r = EYE_RADIUS;
  if (r > w / 2 - 1) {
    r = w / 2 - 1;
  }
  if (r > h / 2 - 1) {
    r = h / 2 - 1;
  }
  if (r < 0) {
    r = 0;
  }
  EyeBox box = {cx - w / 2, y, w, h, r};
  return box;
}
