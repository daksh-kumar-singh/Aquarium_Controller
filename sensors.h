#pragma once
#include <Arduino.h>

namespace sensors {

struct Reading {
  float tempC      = NAN;
  float pH         = NAN;
  bool  levelWet   = false;

  // TCS3200 outputs
  float colorHz    = 0.0f;  // CLEAR channel frequency (Hz)
  float rN         = 0.0f;  // normalized R (0..1)
  float gN         = 0.0f;  // normalized G (0..1)
  float bN         = 0.0f;  // normalized B (0..1)
};

void init();
Reading readAll();

} // namespace sensors