#include "sensors.h"
#include "pins.h"
#include "config.h"
#include "logging.h"

// ---- DS18B20 ----
#include <OneWire.h>
#include <DallasTemperature.h>
static OneWire oneWire(pins::PIN_DS18B20);
static DallasTemperature dallas(&oneWire);

// ---- pH (ADC) ----
// (Gravity BNC adapter → analog voltage → ADC on GPIO34)

// ---- Level switch ----
// (Digital with INPUT_PULLUP; LOW = closed/wet depending on wiring)

// ---- TCS3200 color ----
// (Frequency on COLOR_OUT measured via pulseIn)

namespace sensors {

void init() {
  // Start DS18B20
  dallas.begin();
  // ADC resolution is 12-bit on ESP32 by default; confirm if needed
  analogReadResolution(12);
}

static float readTempC() {
  dallas.requestTemperatures();
  float t = dallas.getTempCByIndex(0);
  if (t < -55.0f || t > 125.0f) { // sanity
    LOGW("Temp read out-of-range: %.2f", t);
  }
  return t + TEMP_OFFSET_C;
}

static float readPH() {
  // Sample/average to reduce noise
  const int N = 64;
  uint32_t acc = 0;
  for (int i = 0; i < N; ++i) acc += analogRead(pins::PIN_PH_ADC);
  float avg = float(acc) / N;     // 0..4095
  // Linear calibration — replace with your 2-point result
  float ph = PH_SLOPE * avg + PH_OFFSET;
  return ph;
}

static bool readLevelWet() {
  // With INPUT_PULLUP, LOW typically means closed switch
  int v = digitalRead(pins::PIN_LEVEL_FLOAT);
  return (v == LOW);
}

static float readColorHz() {
  // Measure period of HIGH pulse; timeout to avoid blocking
  unsigned long period_us = pulseIn(pins::PIN_COLOR_OUT, HIGH, 50000UL);
  if (period_us == 0) return 0.0f;
  return 1e6f / float(period_us);
}

Reading readAll() {
  Reading r;
  r.tempC   = readTempC();
  r.pH      = readPH();
  r.levelWet= readLevelWet();
  r.colorHz = readColorHz();
  return r;
}

} // namespace sensors
