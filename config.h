#pragma once

// ===== Feature toggles =====
#define USE_BLE 1

// ===== Timing =====
#ifndef SENSOR_SAMPLE_PERIOD_MS
#define SENSOR_SAMPLE_PERIOD_MS 2000UL
#endif

// ===== Calibration placeholders (tune later) =====
// pH calibration: pH = slope * (adcAvg) + offset
#define PH_SLOPE   (-0.00300f)  // example placeholder
#define PH_OFFSET  (13.50f)     // example placeholder

// Temperature offset (compare to reference thermometer)
#define TEMP_OFFSET_C (0.00f)