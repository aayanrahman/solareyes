#include "eyes.h"

#include <cstdio>

static int failures = 0;

#define CHECK(cond)                                                          \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);           \
      failures++;                                                            \
    }                                                                        \
  } while (0)

static void title(const char* name) { std::printf("  %s\n", name); }

// Scripted stand-in for esp_random(). Each call returns the next value.
static uint32_t script[8];
static int scriptLen = 0;
static int scriptPos = 0;

static void setScript(uint32_t a, uint32_t b = 0, uint32_t c = 0) {
  script[0] = a;
  script[1] = b;
  script[2] = c;
  scriptLen = 3;
  scriptPos = 0;
}

static uint32_t scripted() {
  uint32_t value = script[scriptPos % scriptLen];
  scriptPos++;
  return value;
}

static bool samePose(const Pose& a, const Pose& b) {
  return a.dxL == b.dxL && a.dxR == b.dxR && a.dyL == b.dyL &&
         a.dyR == b.dyR && a.wL == b.wL && a.wR == b.wR && a.hL == b.hL &&
         a.hR == b.hR && a.browL == b.browL && a.browR == b.browR &&
         a.archL == b.archL && a.archR == b.archR;
}

static void testApproach() {
  title("approach steps toward the target and never overshoots");
  int value = 0;
  approach(value, 5, 2);
  CHECK(value == 2);
  approach(value, 5, 2);
  approach(value, 5, 2);
  CHECK(value == 5);
  approach(value, 5, 2);
  CHECK(value == 5);
  approach(value, -1, 4);
  CHECK(value == 1);
  approach(value, -1, 4);
  CHECK(value == -1);
}

static void testBoot() {
  title("boot sits idle at rest for BOOT_IDLE_MS");
  Eyes eyes;
  eyesInit(eyes, 1000);
  CHECK(eyes.expr == Expr::Idle);
  CHECK(samePose(eyes.shown, REST_POSE));
  CHECK(samePose(eyesTarget(eyes), REST_POSE));

  setScript(50);
  CHECK(!eyesUpdate(eyes, 1000 + BOOT_IDLE_MS - 1, scripted));
  CHECK(eyes.expr == Expr::Idle);
  CHECK(eyesUpdate(eyes, 1000 + BOOT_IDLE_MS, scripted));
  CHECK(eyes.expr == Expr::LookSide);
}

static void testMillisWrap() {
  title("expression timing survives millis() wrapping");
  Eyes eyes;
  uint32_t now = 0xFFFFFF00u;
  eyesInit(eyes, now);
  setScript(20);
  CHECK(!eyesUpdate(eyes, now + 1000, scripted));
  CHECK(eyesUpdate(eyes, now + BOOT_IDLE_MS, scripted));
  CHECK(eyes.expr == Expr::Blink);
  CHECK(eyes.exprEnd == now + BOOT_IDLE_MS + 170);
}

static Expr pick(uint32_t roll) {
  Eyes eyes;
  eyesInit(eyes, 0);
  setScript(roll, 0, 0);
  eyesUpdate(eyes, BOOT_IDLE_MS, scripted);
  return eyes.expr;
}

static void testRotation() {
  title("the roll maps onto the same weights as v0.1");
  CHECK(pick(0) == Expr::Idle);
  CHECK(pick(17) == Expr::Idle);
  CHECK(pick(18) == Expr::Blink);
  CHECK(pick(33) == Expr::Blink);
  CHECK(pick(34) == Expr::LookSide);
  CHECK(pick(57) == Expr::LookSide);
  CHECK(pick(58) == Expr::LookUp);
  CHECK(pick(71) == Expr::LookUp);
  CHECK(pick(72) == Expr::LookDown);
  CHECK(pick(83) == Expr::LookDown);
  CHECK(pick(84) == Expr::Wink);
  CHECK(pick(92) == Expr::Wink);
  CHECK(pick(93) == Expr::Sleepy);
  CHECK(pick(99) == Expr::Sleepy);
  CHECK(pick(100) == Expr::Idle);  // the roll is taken mod 100

  int happy = 0;
  for (uint32_t roll = 0; roll < 100; roll++) {
    if (pick(roll) == Expr::Happy) {
      happy++;
    }
  }
  CHECK(happy == 0);
}

static void testDurations() {
  title("durations and the side and wink choices follow the dice");
  Eyes eyes;
  eyesInit(eyes, 0);
  setScript(0, 279);
  eyesUpdate(eyes, 5000, scripted);
  CHECK(eyes.expr == Expr::Idle);
  CHECK(eyes.exprEnd == 5000 + 420 + 279);

  setScript(40, 0, 0);
  eyesUpdate(eyes, 6000, scripted);
  CHECK(eyes.expr == Expr::LookSide);
  CHECK(eyes.exprEnd == 6000 + 480);
  CHECK(eyes.glanceDir == -1);

  setScript(40, 10, 1);
  eyesUpdate(eyes, 7000, scripted);
  CHECK(eyes.glanceDir == 1);
  CHECK(eyes.exprEnd == 7000 + 490);

  setScript(90, 0);
  eyesUpdate(eyes, 8000, scripted);
  CHECK(eyes.expr == Expr::Wink);
  CHECK(!eyes.winkLeft);
  CHECK(eyes.exprEnd == 8000 + 260);

  setScript(90, 1);
  eyesUpdate(eyes, 9000, scripted);
  CHECK(eyes.winkLeft);
}

static void testSideGlanceMirrors() {
  title("a glance left is the mirror of a glance right");
  Eyes left;
  Eyes right;
  eyesInit(left, 0);
  eyesInit(right, 0);
  left.expr = Expr::LookSide;
  right.expr = Expr::LookSide;
  left.glanceDir = -1;
  right.glanceDir = 1;
  Pose l = eyesTarget(left);
  Pose r = eyesTarget(right);
  CHECK(l.wL == r.wR && l.wR == r.wL);
  CHECK(l.hL == r.hR && l.hR == r.hL);
  CHECK(l.dxL == -r.dxR && l.dxR == -r.dxL);
  CHECK(l.archL == r.archR && l.archR == r.archL);
  CHECK(l.wL > l.wR);  // looking left, the left eye is the near one
}

static void testEasing() {
  title("easing lands exactly on the target pose");
  Eyes eyes;
  eyesInit(eyes, 0);
  eyes.expr = Expr::Sleepy;
  Pose goal = eyesTarget(eyes);
  for (int i = 0; i < 40; i++) {
    eyesStep(eyes);
  }
  CHECK(samePose(eyes.shown, goal));

  // Height moves two pixels a frame, everything else one.
  eyesInit(eyes, 0);
  eyes.expr = Expr::Blink;
  eyesStep(eyes);
  CHECK(eyes.shown.hL == REST_POSE.hL - 2);
  CHECK(eyes.shown.browL == REST_POSE.browL + 1);
}

static void testBob() {
  title("the idle bob is a -2 to 2 triangle wave");
  int lo = 99;
  int hi = -99;
  for (uint32_t t = 0; t < 140 * 8; t += 140) {
    int bob = eyesBob(t);
    lo = bob < lo ? bob : lo;
    hi = bob > hi ? bob : hi;
  }
  CHECK(lo == -2 && hi == 2);
  CHECK(eyesBob(0) == -2);
  CHECK(eyesBob(140 * 4) == 2);
  CHECK(eyesBob(140 * 8) == eyesBob(0));
}

static void testEyeBoxRules() {
  title("every eye box is legal for drawRBox and stays in the blue rows");
  for (int w = 0; w <= 40; w++) {
    for (int h = 0; h <= 40; h++) {
      EyeBox box = eyeBox(64, EYE_Y, w, h);
      CHECK(box.r >= 0);
      CHECK(box.w >= 2 * (box.r + 1));
      CHECK(box.h >= 2 * (box.r + 1) || box.r == 0);
      CHECK(box.h >= 1);
      CHECK(box.y >= BLUE_TOP);
    }
  }
  EyeBox rest = eyeBox(LEFT_X, EYE_Y, 26, 34);
  CHECK(rest.r == EYE_RADIUS);
  CHECK(rest.x == LEFT_X - 13 && rest.y == EYE_Y - 17);

  EyeBox clipped = eyeBox(64, BLUE_TOP + 2, 26, 20);
  CHECK(clipped.y == BLUE_TOP);
  CHECK(clipped.h == 12);
}

static void testEveryPoseFitsThePanel() {
  title("every expression, at either end of the bob, fits on the panel");
  const Expr all[] = {Expr::Idle,     Expr::Blink, Expr::LookSide,
                      Expr::LookUp,   Expr::LookDown, Expr::Happy,
                      Expr::Wink,     Expr::Sleepy};
  for (Expr expr : all) {
    for (int dir = -1; dir <= 1; dir += 2) {
      Eyes eyes;
      eyesInit(eyes, 0);
      eyes.expr = expr;
      eyes.glanceDir = (int8_t)dir;
      eyes.winkLeft = dir < 0;
      Pose p = eyesTarget(eyes);
      for (int bob = -2; bob <= 2; bob += 4) {
        EyeBox l = eyeBox(LEFT_X + p.dxL, EYE_Y + p.dyL + bob, p.wL, p.hL);
        EyeBox r = eyeBox(RIGHT_X + p.dxR, EYE_Y + p.dyR + bob, p.wR, p.hR);
        CHECK(l.x >= 0 && l.x + l.w <= SCREEN_W);
        CHECK(r.x >= 0 && r.x + r.w <= SCREEN_W);
        CHECK(l.y >= BLUE_TOP && l.y + l.h <= SCREEN_H);
        CHECK(r.y >= BLUE_TOP && r.y + r.h <= SCREEN_H);
        CHECK(l.x + l.w < r.x);  // the eyes never touch
      }
    }
  }
}

int main() {
  std::printf("eyes\n");
  testApproach();
  testBoot();
  testMillisWrap();
  testRotation();
  testDurations();
  testSideGlanceMirrors();
  testEasing();
  testBob();
  testEyeBoxRules();
  testEveryPoseFitsThePanel();
  if (failures) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("all checks passed\n");
  return 0;
}
