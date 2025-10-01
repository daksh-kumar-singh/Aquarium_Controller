#pragma once

// How often to read all sensors (ms)
#define SENSOR_SAMPLE_PERIOD_MS 1000UL

// ====== pH Calibration (2-point line: pH = m*V + b) ======
// Set these after you calibrate with pH 4/7/10 buffers.
#define PH_SLOPE_M   -5.70f   // placeholder slope (units: pH per Volt)
#define PH_OFFSET_B   21.34f  // placeholder offset

// ====== TCS3200 Options ======
#define TCS_USE_FREQUENCY_MODE   1   // 1=Hz via HIGH+LOW; 0=HIGH pulse µs
#define TCS_MEDIAN_SAMPLES       7   // odd number 1..21 recommended
#define TCS_START_20_PERCENT     1   // 1=20% scaling, 0=100%

// Replace with your measured calibration later
#if TCS_USE_FREQUENCY_MODE
  #define RED_FREQ_DARK     1400.0f
  #define RED_FREQ_BRIGHT   6500.0f
  #define GRN_FREQ_DARK      550.0f
  #define GRN_FREQ_BRIGHT   2500.0f
  #define BLU_FREQ_DARK      400.0f
  #define BLU_FREQ_BRIGHT   2000.0f
#else
  #define RED_DARK_US        400
  #define RED_BRIGHT_US       90
  #define GRN_DARK_US       1050
  #define GRN_BRIGHT_US      235
  #define BLU_DARK_US       1350
  #define BLU_BRIGHT_US      300
#endif