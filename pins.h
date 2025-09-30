#pragma once
#include <Arduino.h>

namespace pins {
// Adjust as you wire your hardware
static constexpr int PIN_DS18B20     = 4;   // 1-Wire data
static constexpr int PIN_LEVEL_FLOAT = 5;   // float switch (uses INPUT_PULLUP)
static constexpr int PIN_PH_ADC      = 34;  // analog input
static constexpr int PIN_COLOR_OUT   = 14;  // TCS3200 frequency out

inline void initPins() {
  pinMode(PIN_LEVEL_FLOAT, INPUT_PULLUP);
  pinMode(PIN_COLOR_OUT, INPUT);
  // ADC & 1-Wire pins are configured by their libs when used
}
} // namespace pins
