#include <Arduino.h>
#include "config.h"
#include "pins.h"
#include "logging.h"
#include "sensors.h"

static unsigned long lastSampleMs = 0;

void setup() {
  Serial.begin(115200);
  delay(150);
  LOGI("Aquarium_Controller booting...");

  pins::initPins();
  sensors::init();

  LOGI("Init complete. Sampling every %lu ms", (unsigned long)SENSOR_SAMPLE_PERIOD_MS);
}

void loop() {
  const unsigned long now = millis();
  if (now - lastSampleMs >= SENSOR_SAMPLE_PERIOD_MS) {
    lastSampleMs = now;

    sensors::Reading r = sensors::readAll();

    LOGI("T=%.2fC  pH=%.2f  Level=%s  C=%.0f Hz  RGBn=(%.2f, %.2f, %.2f)",
         r.tempC,
         r.pH,
         (r.levelWet ? "WET" : "DRY"),
         r.colorHz,
         r.rN, r.gN, r.bN);
  }
}