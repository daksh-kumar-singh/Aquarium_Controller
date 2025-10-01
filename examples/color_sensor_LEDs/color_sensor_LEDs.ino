/*
  TCS3200 Standalone Bring-up on ESP32 (3.3V logic)
  Features:
    - Power gating via S0/S1 (OFF by default; button press toggles ON/OFF)
    - Median-of-N sampling
    - Frequency (HIGH+LOW) or pulse-width measurement
    - Per-channel black/white mapping
    - Optional normalized RGB ratios
*/

#include <Arduino.h>

// -------------------- PIN MAP (ESP32) --------------------
const int S0            = 16;  // frequency scale / power gate
const int S1            = 17;  // frequency scale / power gate
const int S2            = 18;  // color filter select
const int S3            = 19;  // color filter select
const int COLOR_OUT_PIN = 34;  // TCS3200 OUT (digital freq) | input-only is fine

// Button to toggle sensor power (GND when pressed)
const int BUTTON_PIN    = 13;  // choose a free GPIO (avoid boot-strapping pins)
const unsigned long DEBOUNCE_MS = 50;

// If you wired OE: active LOW enable. (Tie OE to GND if unused)
// const int OE = XX;

// -------------------- USER OPTIONS -----------------------
#define USE_FREQUENCY_MODE     1   // 1=Hz (HIGH+LOW), 0=pulse width (HIGH)
#define MEDIAN_SAMPLES         7   // odd, up to ~21
#define START_WITH_20_PERCENT  1   // 1 = 20% scaling, 0 = 100% scaling
#define PRINT_NORMALIZED       1   // print rN/gN/bN in [0..1]

// ---------------- CALIBRATION CONSTANTS ------------------
// Replace with your measured BLACK/WHITE numbers per channel.
#if USE_FREQUENCY_MODE
  float RED_FREQ_DARK = 1400.0f, RED_FREQ_BRIGHT = 6500.0f;
  float GRN_FREQ_DARK =  550.0f, GRN_FREQ_BRIGHT = 2500.0f;
  float BLU_FREQ_DARK =  400.0f, BLU_FREQ_BRIGHT = 2000.0f;
#else
  int   RED_DARK_US   =   400,   RED_BRIGHT_US   =   90;
  int   GRN_DARK_US   =  1050,   GRN_BRIGHT_US   =  235;
  int   BLU_DARK_US   =  1350,   BLU_BRIGHT_US   =  300;
#endif

// ----------------- TCS3200 HELPERS -----------------------
static inline void tcs_scale_off()   { digitalWrite(S0, LOW);  digitalWrite(S1, LOW);  } // power down
static inline void tcs_scale_20()    { digitalWrite(S0, HIGH); digitalWrite(S1, LOW);  }
static inline void tcs_scale_100()   { digitalWrite(S0, HIGH); digitalWrite(S1, HIGH); }

static inline void tcs_filter_red()   { digitalWrite(S2, LOW);  digitalWrite(S3, LOW);  } // 00
static inline void tcs_filter_blue()  { digitalWrite(S2, LOW);  digitalWrite(S3, HIGH); } // 01
static inline void tcs_filter_clear() { digitalWrite(S2, HIGH); digitalWrite(S3, LOW);  } // 10
static inline void tcs_filter_green() { digitalWrite(S2, HIGH); digitalWrite(S3, HIGH); } // 11

static inline unsigned long readPulseHigh(int pin, unsigned long timeout_us = 60000UL) {
  return pulseIn(pin, HIGH, timeout_us);
}

static float measureHz(unsigned long timeout_us = 60000UL) {
  unsigned long th = pulseIn(COLOR_OUT_PIN, HIGH, timeout_us);
  unsigned long tl = pulseIn(COLOR_OUT_PIN, LOW,  timeout_us);
  if (th == 0 || tl == 0) return 0.0f;
  float period_us = float(th + tl);
  return 1e6f / period_us;
}

template<typename T>
T medianOfN(T* arr, int n) {
  for (int i = 1; i < n; ++i) {
    T key = arr[i]; int j = i - 1;
    while (j >= 0 && arr[j] > key) { arr[j+1] = arr[j]; --j; }
    arr[j+1] = key;
  }
  return arr[n/2];
}

static unsigned long medianPulseUs() {
  unsigned long a[MEDIAN_SAMPLES];
  for (int i = 0; i < MEDIAN_SAMPLES; ++i) a[i] = readPulseHigh(COLOR_OUT_PIN);
  return medianOfN(a, MEDIAN_SAMPLES);
}

static float medianHz() {
  float a[MEDIAN_SAMPLES];
  for (int i = 0; i < MEDIAN_SAMPLES; ++i) a[i] = measureHz();
  return medianOfN(a, MEDIAN_SAMPLES);
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

// Central power control: OFF = S0/S1 LOW, ON = 20% or 100%
static void tcs_power(bool on) {
  if (!on) { tcs_scale_off(); return; }
#if START_WITH_20_PERCENT
  tcs_scale_20();
#else
  tcs_scale_100();
#endif
}

// ------------------------- STATE -------------------------
static bool sensorOn = false;
static int  lastBtnState = HIGH;     // INPUT_PULLUP: HIGH when not pressed
static unsigned long lastDebounceMs = 0;

// ------------------------- SETUP -------------------------
void setup() {
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(COLOR_OUT_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // If you wired OE: pinMode(OE, OUTPUT); digitalWrite(OE, LOW);

  // Start sensor OFF
  tcs_power(false);

  Serial.begin(115200);
  delay(200);
  Serial.println(F("TCS3200: OFF initially; press button to toggle ON/OFF."));
}

// -------------------------- LOOP -------------------------
void loop() {
  // ---- Button edge-detect with debounce ----
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastBtnState) {
    lastDebounceMs = millis();
    lastBtnState = reading;
  }

  // On stable LOW after debounce window, toggle sensor
  if (reading == LOW && (millis() - lastDebounceMs) > DEBOUNCE_MS) {
    // Wait for release to avoid multiple toggles on a long press
    while (digitalRead(BUTTON_PIN) == LOW) { delay(1); }

    sensorOn = !sensorOn;
    tcs_power(sensorOn);
    Serial.println(sensorOn ? F("TCS3200: ON (button)") : F("TCS3200: OFF (button)"));
    // Small settle after power change
    delay(50);
  }

  if (!sensorOn) {
    Serial.println(F("TCS3200 OFF (S0/S1 LOW)"));
    delay(250);
    return;
  }

  // ---- Measure channels ----
  tcs_filter_clear(); delay(10);
#if USE_FREQUENCY_MODE
  float cHz = medianHz();
#else
  unsigned long cPW = medianPulseUs();
#endif

  tcs_filter_red(); delay(10);
#if USE_FREQUENCY_MODE
  float rHz = medianHz();
#else
  unsigned long rPW = medianPulseUs();
#endif

  tcs_filter_green(); delay(10);
#if USE_FREQUENCY_MODE
  float gHz = medianHz();
#else
  unsigned long gPW = medianPulseUs();
#endif

  tcs_filter_blue(); delay(10);
#if USE_FREQUENCY_MODE
  float bHz = medianHz();
#else
  unsigned long bPW = medianPulseUs();
#endif

  // ---- Map to 0..255 ----
  int R, G, B;
#if USE_FREQUENCY_MODE
  R = map255_hz(rHz, RED_FREQ_DARK, RED_FREQ_BRIGHT);
  G = map255_hz(gHz, GRN_FREQ_DARK, GRN_FREQ_BRIGHT);
  B = map255_hz(bHz, BLU_FREQ_DARK, BLU_FREQ_BRIGHT);
#else
  R = map255_us(rPW, RED_DARK_US, RED_BRIGHT_US);
  G = map255_us(gPW, GRN_DARK_US, GRN_BRIGHT_US);
  B = map255_us(bPW, BLU_DARK_US, BLU_BRIGHT_US);
#endif

  // ---- Optional normalized ratios ----
#if PRINT_NORMALIZED
  float sum = float(R + G + B);
  float rN = (sum > 0) ? (float)R / sum : 0.0f;
  float gN = (sum > 0) ? (float)G / sum : 0.0f;
  float bN = (sum > 0) ? (float)B / sum : 0.0f;
#endif

  // ---- Print ----
#if USE_FREQUENCY_MODE
  Serial.print(F("RAW Hz  C:")); Serial.print(cHz, 0);
  Serial.print(F("  R:"));       Serial.print(rHz, 0);
  Serial.print(F("  G:"));       Serial.print(gHz, 0);
  Serial.print(F("  B:"));       Serial.print(bHz, 0);
#else
  Serial.print(F("RAW us  C:")); Serial.print(cPW);
  Serial.print(F("  R:"));       Serial.print(rPW);
  Serial.print(F("  G:"));       Serial.print(gPW);
  Serial.print(F("  B:"));       Serial.print(bPW);
#endif

  Serial.print(F("  |  MAPPED  R:")); Serial.print(R);
  Serial.print(F("  G:"));             Serial.print(G);
  Serial.print(F("  B:"));             Serial.print(B);

#if PRINT_NORMALIZED
  Serial.print(F("  |  NORM rN:")); Serial.print(rN, 2);
  Serial.print(F(" gN:"));            Serial.print(gN, 2);
  Serial.print(F(" bN:"));            Serial.println(bN, 2);
#else
  Serial.println();
#endif

  delay(250);
}