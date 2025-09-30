#pragma once
#include "sensors.h"

namespace ble_if {
void init(const char* deviceName);
void notifyReading(const sensors::Reading& r);
void poll();
} // namespace ble_if
