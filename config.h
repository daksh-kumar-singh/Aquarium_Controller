#pragma once

// How often to read all sensors (ms)
#define SENSOR_SAMPLE_PERIOD_MS 1000UL

// ====== pH Calibration (2-point line: pH = m*V + b) ======
#define PH_SLOPE_M   12.947211f   // slope (pH/Volt)
#define PH_OFFSET_B  -7.582004f  // offset

// ====== TCS3200 Options ======
#define USE_FREQ_MODE          1   // 1=Hz via HIGH+LOW; 0=HIGH pulse µs
#define TCS_MEDIAN_SAMPLES     7   // odd number 1..21 recommended
#define TCS_START_20_PERCENT   1   // 1=20% scaling, 0=100%

// ====== TCS3200 Calibration (declared here, defined in sensors.cpp) ======
#if USE_FREQ_MODE
  extern float RED_FREQ_DARK;    extern float RED_FREQ_BRIGHT;
  extern float GRN_FREQ_DARK;    extern float GRN_FREQ_BRIGHT;
  extern float BLU_FREQ_DARK;    extern float BLU_FREQ_BRIGHT;
#else
  extern int   RED_DARK_US;      extern int   RED_BRIGHT_US;
  extern int   GRN_DARK_US;      extern int   GRN_BRIGHT_US;
  extern int   BLU_DARK_US;      extern int   BLU_BRIGHT_US;
#endif