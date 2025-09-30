#pragma once
#include <Arduino.h>

namespace sensors {

struct Reading {
  float tempC;     // DS18B20
  float pH;        // pH via ADC mapping
  bool  levelWet;  // float switch
  float colorHz;   // TCS3200 frequency estimate
};

void init();
Reading readAll();

} // namespace sensors
