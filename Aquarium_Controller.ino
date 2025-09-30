#include <Arduino.h>
#include "config.h"
#include "pins.h"
#include "logging.h"
#include "sensors.h"
#include "ble_if.h"

void setup() {
  Serial.begin(115200);
  delay(100);
  LOGI("Aquarium_Controller booting...");

  // Hardware & subsystem init
  pins::initPins();
  sensors::init();
#if USE_BLE
  ble_if::init("TankMonitor");
#endif

  LOGI("Setup complete.");
}

static unsigned long lastSampleMs = 0;

void loop() {
  const unsigned long now = millis();

  // Periodic sensor read
  if (now - lastSampleMs >= SENSOR_SAMPLE_PERIOD_MS) {
    lastSampleMs = now;
    sensors::Reading r = sensors::readAll();
    LOGI("T=%.2fC, pH=%.2f, Level=%s, ColorHz=%.0f",
         r.tempC, r.pH, (r.levelWet ? "WET" : "DRY"), r.colorHz);

#if USE_BLE
    ble_if::notifyReading(r);
#endif
  }

#if USE_BLE
  ble_if::poll();
#endif
}
