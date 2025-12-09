#pragma once
#include <Arduino.h>

namespace pins {

// ---- DS18B20 Temperature (OneWire) ----
static constexpr int ONE_WIRE_BUS = 34;   // GPIO26 (pull-up 4.7k to 3V3)

// ---- pH Analog (ADC1 only: 32..39) ----
static constexpr int PH_ADC = 35;         // GPIO35 (input-only)

// ---- Float switch (with internal pull-up) ----
static constexpr int LEVEL_PIN = 2;       // GPIO5

// ---- TCS3200 Color Sensor ----
static constexpr int TCS_S0    = 19;      // freq scale / power
static constexpr int TCS_S1    = 18;      // freq scale / power
static constexpr int TCS_S2    = 17;      // filter select
static constexpr int TCS_S3    = 16;      // filter select
static constexpr int TCS_OUT   = 21;      // freq output (input-only OK)

// ---- LCD I2C ----
static constexpr int LCD_SDA   = 13;      // I2C SDA
static constexpr int LCD_SCL   = 12;      // I2C SCL

inline void initPins() {
  // Float switch
  pinMode(LEVEL_PIN, INPUT_PULLUP);

  // TCS control & input
  pinMode(TCS_S0, OUTPUT);
  pinMode(TCS_S1, OUTPUT);
  pinMode(TCS_S2, OUTPUT);
  pinMode(TCS_S3, OUTPUT);
  pinMode(TCS_OUT, INPUT);

  // DS18B20 data line helper pull-up (external 4.7k still required)
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
}

} // namespace pins