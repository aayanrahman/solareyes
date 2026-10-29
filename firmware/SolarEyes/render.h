#pragma once

#include <stdint.h>

#include "eyes.h"

// Draws the current pose into the U8g2 buffer in RAM, blanks the yellow
// rows, and pushes the frame to the panel.
void renderFrame(const Eyes& eyes, uint32_t nowMs);
