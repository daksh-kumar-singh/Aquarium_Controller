#include "ble_if.h"
#include "logging.h"
#include "config.h"

#if USE_BLE
  #include <BLEDevice.h>
  #include <BLEUtils.h>
  #include <BLEServer.h>

  static BLEServer*       gServer   = nullptr;
  static BLEService*      gService  = nullptr;
  static BLECharacteristic* gCharT  = nullptr; // temperature
  static BLECharacteristic* gCharPH = nullptr; // pH
  static BLECharacteristic* gCharLV = nullptr; // level
  static BLECharacteristic* gCharCL = nullptr; // color Hz

  // Simple UUIDs (use proper 128-bit in production)
  static BLEUUID SVC_UUID((uint16_t)0x181A); // Environmental Sensing
  static BLEUUID TEMP_UUID((uint16_t)0x2A6E);
  static BLEUUID PH_UUID  ((uint16_t)0x2A6D); // reuse placeholder
  static BLEUUID LV_UUID  ((uint16_t)0x2A56); // Digital in/out placeholder
  static BLEUUID CL_UUID  ((uint16_t)0x2A70); // UV index placeholder (repurpose)

#endif

namespace ble_if {

void init(const char* deviceName) {
#if USE_BLE
  BLEDevice::init(deviceName);
  gServer  = BLEDevice::createServer();
  gService = gServer->createService(SVC_UUID);

  gCharT = gService->createCharacteristic(TEMP_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  gCharPH= gService->createCharacteristic(PH_UUID,   BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  gCharLV= gService->createCharacteristic(LV_UUID,   BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  gCharCL= gService->createCharacteristic(CL_UUID,   BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  gService->start();
  BLEDevice::startAdvertising();
  LOGI("BLE advertising as '%s'", deviceName);
#else
  (void)deviceName;
#endif
}

void notifyReading(const sensors::Reading& r) {
#if USE_BLE
  // Send compact text payloads for now (simple to test with BLE apps)
  char buf[48];

  snprintf(buf, sizeof(buf), "%.2f", r.tempC);
  gCharT->setValue((uint8_t*)buf, strlen(buf));
  gCharT->notify();

  snprintf(buf, sizeof(buf), "%.2f", r.pH);
  gCharPH->setValue((uint8_t*)buf, strlen(buf));
  gCharPH->notify();

  snprintf(buf, sizeof(buf), "%d", r.levelWet ? 1 : 0);
  gCharLV->setValue((uint8_t*)buf, strlen(buf));
  gCharLV->notify();

  snprintf(buf, sizeof(buf), "%.0f", r.colorHz);
  gCharCL->setValue((uint8_t*)buf, strlen(buf));
  gCharCL->notify();
#else
  (void)r;
#endif
}

void poll() {
  // no-op for now; hook for future server callbacks
}

} // namespace ble_if
