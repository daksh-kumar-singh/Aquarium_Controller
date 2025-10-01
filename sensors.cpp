#include "sensors.h"
#include "pins.h"
#include "config.h"
#include "logging.h"

#include <OneWire.h>
#include <DallasTemperature.h>

// ---------------- DS18B20 ----------------
static OneWire oneWire(pins::ONE_WIRE_BUS);
static DallasTemperature ds18b20(&oneWire);

// ---------------- pH (ADC) ---------------
static float readPH_Volts() {
  // Average multiple samples to reduce noise
  const int N = 64;
  uint32_t acc = 0;
  for (int i = 0; i < N; ++i) {
    acc += analogRead(pins::PH_ADC);
    delayMicroseconds(150);
  }
  float avgRaw = float(acc) / N;    // 0..4095 (ADC1 default 12-bit)
  // ESP32 ADC is non-linear; for first pass, scale linearly by reference 3.3V
  float volts = (avgRaw / 4095.0f) * 3.3f;
  return volts;
}

static float convertVoltsToPH(float volts) {
  // Linear mapping: pH = m*V + b   (set m,b in config.h after 2-point calibration)
  return PH_SLOPE_M * volts + PH_OFFSET_B;
}

// -------------- Float switch -------------
static bool readLevelWet() {
  // INPUT_PULLUP: LOW when switch closed (wet), HIGH when open (dry)
  return digitalRead(pins::LEVEL_PIN) == LOW;
}

// -------------- TCS3200 Helpers ----------
static inline void tcs_scale_off()   { digitalWrite(pins::TCS_S0, LOW);  digitalWrite(pins::TCS_S1, LOW);  }
static inline void tcs_scale_20()    { digitalWrite(pins::TCS_S0, HIGH); digitalWrite(pins::TCS_S1, LOW);  }
static inline void tcs_scale_100()   { digitalWrite(pins::TCS_S0, HIGH); digitalWrite(pins::TCS_S1, HIGH); }

static inline void tcs_filter_red()   { digitalWrite(pins::TCS_S2, LOW);  digitalWrite(pins::TCS_S3, LOW);  } // 00
static inline void tcs_filter_blue()  { digitalWrite(pins::TCS_S2, LOW);  digitalWrite(pins::TCS_S3, HIGH); } // 01
static inline void tcs_filter_clear() { digitalWrite(pins::TCS_S2, HIGH); digitalWrite(pins::TCS_S3, LOW);  } // 10
static inline void tcs_filter_green() { digitalWrite(pins::TCS_S2, HIGH); digitalWrite(pins::TCS_S3, HIGH); } // 11

static inline unsigned long pulseHigh(unsigned long timeout_us = 60000UL) {
  return pulseIn(pins::TCS_OUT, HIGH, timeout_us);
}
static float measureHz(unsigned long timeout_us = 60000UL) {
  unsigned long th = pulseIn(pins::TCS_OUT, HIGH, timeout_us);
  unsigned long tl = pulseIn(pins::TCS_OUT, LOW,  timeout_us);
  if (th == 0 || tl == 0) return 0.0f;
  float period_us = float(th + tl);
  return 1e6f / period_us;
}

template<typename T>
static T medianOfN(T* arr, int n) {
  for (int i = 1; i < n; ++i) {
    T key = arr[i]; int j = i - 1;
    while (j >= 0 && arr[j] > key) { arr[j+1] = arr[j]; --j; }
    arr[j+1] = key;
  }
  return arr[n/2];
}

static unsigned long medianPW() {
  unsigned long a[TCS_MEDIAN_SAMPLES];
  for (int i = 0; i < TCS_MEDIAN_SAMPLES; ++i) a[i] = pulseHigh();
  return medianOfN(a, TCS_MEDIAN_SAMPLES);
}
static float medianHz() {
  float a[TCS_MEDIAN_SAMPLES];
  for (int i = 0; i < TCS_MEDIAN_SAMPLES; ++i) a[i] = measureHz();
  return medianOfN(a, TCS_MEDIAN_SAMPLES);
}

static int map255_us(unsigned long pw, int dark_us, int bright_us) {
  long v = map((long)pw, dark_us, bright_us, 0, 255); // larger→0, smaller→255
  if (v < 0) v = 0; if (v > 255) v = 255;
  return (int)v;
}
static int map255_hz(float f, float f_dark, float f_bright) {
  if (f_bright <= f_dark) return 0;
  float v = (f - f_dark) / (f_bright - f_dark) * 255.0f;
  if (v < 0) v = 0; if (v > 255) v = 255;
  return (int)v;
}

static void tcs_power(bool on) {
  if (!on) { tcs_scale_off(); return; }
#if TCS_START_20_PERCENT
  tcs_scale_20();
#else
  tcs_scale_100();
#endif
}

// -------------- Public API ---------------
namespace sensors {

void init() {
  // ADC resolution is 12-bit by default on ESP32 Arduino core for ADC1.
  // If you need different attenuation/scaling, use analogSetPinAttenuation().

  // Temp
  ds18b20.begin();

  // TCS3200 OFF initially; caller may change later
  tcs_power(true);  // enable by default for periodic reads
}

static void tcs_read_norm(float& rN, float& gN, float& bN, float& cHzOut) {
  // CLEAR
  tcs_filter_clear(); delay(10);
#if TCS_USE_FREQUENCY_MODE
  float cHz = medianHz();
#else
  unsigned long cPW = medianPW();
#endif

  // RED
  tcs_filter_red(); delay(10);
#if TCS_USE_FREQUENCY_MODE
  float rHz = medianHz();
#else
  unsigned long rPW = medianPW();
#endif

  // GREEN
  tcs_filter_green(); delay(10);
#if TCS_USE_FREQUENCY_MODE
  float gHz = medianHz();
#else
  unsigned long gPW = medianPW();
#endif

  // BLUE
  tcs_filter_blue(); delay(10);
#if TCS_USE_FREQUENCY_MODE
  float bHz = medianHz();
#else
  unsigned long bPW = medianPW();
#endif

#if TCS_USE_FREQUENCY_MODE
  // Map to 0..255 (per-channel), then normalize to 0..1
  int R = map255_hz(rHz, RED_FREQ_DARK, RED_FREQ_BRIGHT);
  int G = map255_hz(gHz, GRN_FREQ_DARK, GRN_FREQ_BRIGHT);
  int B = map255_hz(bHz, BLU_FREQ_DARK, BLU_FREQ_BRIGHT);
  float sum = float(R + G + B);
  rN = (sum > 0) ? (float)R / sum : 0.0f;
  gN = (sum > 0) ? (float)G / sum : 0.0f;
  bN = (sum > 0) ? (float)B / sum : 0.0f;
  cHzOut = cHz;
#else
  int R = map255_us(rPW, RED_DARK_US, RED_BRIGHT_US);
  int G = map255_us(gPW, GRN_DARK_US, GRN_BRIGHT_US);
  int B = map255_us(bPW, BLU_DARK_US, BLU_BRIGHT_US);
  float sum = float(R + G + B);
  rN = (sum > 0) ? (float)R / sum : 0.0f;
  gN = (sum > 0) ? (float)G / sum : 0.0f;
  bN = (sum > 0) ? (float)B / sum : 0.0f;
  // For pulse-width mode, still report CLEAR as an equivalent "Hz" field
  cHzOut = (cPW > 0) ? (1e6f / (float)cPW) : 0.0f;
#endif
}

Reading readAll() {
  Reading r;

  // Temperature (DS18B20)
  ds18b20.requestTemperatures();
  r.tempC = ds18b20.getTempCByIndex(0);

  // pH (ADC)
  float v = readPH_Volts();
  r.pH = convertVoltsToPH(v);

  // Water level
  r.levelWet = readLevelWet();

  // Color sensor
  tcs_read_norm(r.rN, r.gN, r.bN, r.colorHz);

  return r;
}

} // namespace sensors