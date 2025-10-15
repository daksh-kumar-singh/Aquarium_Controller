/*
  Aquarium_Controller.ino
  Reads DS18B20 (GPIO26), pH (ADC GPIO35), Float (GPIO5), TCS3200,
  and shows compact status on a 1602 I2C LCD (SDA=21, SCL=22).
  Includes a Serial calibration console for pH & TCS3200.

  Requires libraries:
    - OneWire
    - DallasTemperature
    - LiquidCrystal_I2C
*/

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "pins.h"
#include "config.h"
#include "logging.h"
#include "sensors.h"

// -------------------- Push Button System Control --------------------
static const int BUTTON_PIN = 13;     // Use GPIO13 (change if needed)
static bool systemEnabled = true;     // System starts ON
static bool lastButtonState = HIGH;
static uint32_t lastDebounceTime = 0;
static const uint32_t debounceDelay = 200; // ms

static void initButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Push button initialized (LOW = pressed)");
}

static void updateButton(LiquidCrystal_I2C* lcd) {
  static bool lastPhysicalState = HIGH;   // raw last read
  static bool stableState = HIGH;         // debounced state

  bool currentReading = digitalRead(BUTTON_PIN);

  // Debounce: update only if reading changed
  if (currentReading != lastPhysicalState) {
    lastDebounceTime = millis();
  }
  lastPhysicalState = currentReading;

  // Stable for longer than debounceDelay → accept new state
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentReading != stableState) {
      stableState = currentReading;

      // Button press (LOW) toggles system state
      if (stableState == LOW) {
        systemEnabled = !systemEnabled;
        Serial.printf("System toggled %s\n", systemEnabled ? "ON" : "OFF");

        lcd->clear();
        if (systemEnabled) {
          lcd->setCursor(0, 0); lcd->print("System: ON       ");
          lcd->setCursor(0, 1); lcd->print("Resuming...      ");
        } else {
          lcd->setCursor(0, 0); lcd->print("System: OFF      ");
          lcd->setCursor(0, 1); lcd->print("Press Btn to ON  ");
        }
        delay(500);
      }
    }
  }
}

// -------------------- LCD helpers --------------------
static LiquidCrystal_I2C* lcd = nullptr;
static uint8_t lcdAddr = 0x27;

static bool probeI2C(uint8_t a) { Wire.beginTransmission(a); return Wire.endTransmission() == 0; }

static void lcdPrint16(uint8_t col, uint8_t row, const String& s) {
  lcd->setCursor(col, row);
  String t = s;
  if (t.length() < 16) {
    uint8_t need = 16 - t.length();
    for (uint8_t i = 0; i < need; i++) t += ' ';
  } else if (t.length() > 16) {
    t.remove(16);
  }
  lcd->print(t);
}

// ---------------- median helper (global scope) ----------------
template<typename T>
T medianN(T* a, int n) {
  for (int i = 1; i < n; i++) {
    T key = a[i];
    int j = i - 1;
    while (j >= 0 && a[j] > key) { a[j + 1] = a[j]; j--; }
    a[j + 1] = key;
  }
  return a[n / 2];
}

// ================= CALIBRATION CONSOLE =================

// pH raw volts (averaged)
static float readPHVoltsAvg(int samples = 64) {
  uint32_t acc = 0;
  for (int i = 0; i < samples; i++) { acc += analogRead(pins::PH_ADC); delayMicroseconds(150); }
  float avg = acc / float(samples);          // 0..4095
  return avg * (3.3f / 4095.0f);             // Volts (simple linear scale)
}

// ---- TCS3200 raw helpers (same pins as sensors.cpp) ----
static inline void tcs_scale_off()   { digitalWrite(pins::TCS_S0, LOW);  digitalWrite(pins::TCS_S1, LOW);  }
static inline void tcs_scale20()     { digitalWrite(pins::TCS_S0, HIGH); digitalWrite(pins::TCS_S1, LOW);  }
static inline void tcs_scale100()    { digitalWrite(pins::TCS_S0, HIGH); digitalWrite(pins::TCS_S1, HIGH); }

static inline void tcs_red()   { digitalWrite(pins::TCS_S2, LOW);  digitalWrite(pins::TCS_S3, LOW);  } // 00
static inline void tcs_blue()  { digitalWrite(pins::TCS_S2, LOW);  digitalWrite(pins::TCS_S3, HIGH); } // 01
static inline void tcs_clear() { digitalWrite(pins::TCS_S2, HIGH); digitalWrite(pins::TCS_S3, LOW);  } // 10
static inline void tcs_green() { digitalWrite(pins::TCS_S2, HIGH); digitalWrite(pins::TCS_S3, HIGH); } // 11

static unsigned long pulseHigh(unsigned long to = 60000UL) { return pulseIn(pins::TCS_OUT, HIGH, to); }
static float measureHz(unsigned long to = 60000UL) {
  unsigned long th = pulseIn(pins::TCS_OUT, HIGH, to);
  unsigned long tl = pulseIn(pins::TCS_OUT, LOW,  to);
  if (!th || !tl) return 0.0f;
  return 1e6f / float(th + tl);
}

static float  medianHz(int n = TCS_MEDIAN_SAMPLES) {
  if (n < 1) n = 1; if (n > 21) n = 21;
  float a[21];
  for (int i = 0; i < n; i++) a[i] = measureHz();
  return medianN(a, n);
}
static unsigned long medianUs(int n = TCS_MEDIAN_SAMPLES) {
  if (n < 1) n = 1; if (n > 21) n = 21;
  unsigned long a[21];
  for (int i = 0; i < n; i++) a[i] = pulseHigh();
  return medianN(a, n);
}

// Capture buffers for calibration
static bool phHasA = false, phHasB = false;
static float phV1 = 0, phP1 = 0, phV2 = 0, phP2 = 0;

static bool tcsHasBlack = false, tcsHasWhite = false;
#if USE_FREQ_MODE
  static float r_blk = 0, g_blk = 0, b_blk = 0;
  static float r_wht = 0, g_wht = 0, b_wht = 0;
#else
  static unsigned long r_blk = 0, g_blk = 0, b_blk = 0;
  static unsigned long r_wht = 0, g_wht = 0, b_wht = 0;
#endif

static void printHelp() {
  Serial.println();
  Serial.println(F("=== Calibration Console ==="));
  Serial.println(F("[h] help"));
  Serial.println(F("[A] pH point A: measure volts in buffer, then enter known pH"));
  Serial.println(F("[B] pH point B: measure volts in 2nd buffer, then enter known pH"));
  Serial.println(F("[k] TCS BLACK  capture (hold at black target)"));
  Serial.println(F("[w] TCS WHITE  capture (hold at white target)"));
  Serial.println(F("[r] One-shot RAW print (pH volts + TCS raw)"));
  Serial.println(F("Paste the suggested lines back into config.h and sensors.cpp"));
  Serial.println();
}

static void doPHCapture(char which) {
  float v = readPHVoltsAvg();
  Serial.printf("Measured pH voltage = %.4f V. Enter known pH (e.g., 7.00) and press Enter...\n", v);
  while (!Serial.available()) { delay(5); }
  float p = Serial.parseFloat();
  if (which == 'A') { phV1 = v; phP1 = p; phHasA = true; }
  else              { phV2 = v; phP2 = p; phHasB = true; }
  Serial.printf("Saved point %c: (V=%.4f, pH=%.4f)\n", which, v, p);

  if (phHasA && phHasB) {
    if (fabs(phV2 - phV1) < 1e-6f) {
      Serial.println(F("!! Voltages identical; take points at different buffers."));
    } else {
      float m = (phP2 - phP1) / (phV2 - phV1);
      float b = phP1 - m * phV1;
      Serial.println(F("\n=== Suggested lines for config.h ==="));
      Serial.printf("#define PH_SLOPE_M   %.6ff\n", m);
      Serial.printf("#define PH_OFFSET_B  %.6ff\n", b);
      Serial.println(F("====================================\n"));
    }
  }
}

static void readTCSRawOnce(
#if USE_FREQ_MODE
  float &r, float &g, float &b
#else
  unsigned long &r, unsigned long &g, unsigned long &b
#endif
){
#if TCS_START_20_PERCENT
  tcs_scale20();
#else
  tcs_scale100();
#endif
  tcs_red();   delay(8);
#if USE_FREQ_MODE
  r = medianHz();
#else
  r = medianUs();
#endif
  tcs_green(); delay(8);
#if USE_FREQ_MODE
  g = medianHz();
#else
  g = medianUs();
#endif
  tcs_blue();  delay(8);
#if USE_FREQ_MODE
  b = medianHz();
#else
  b = medianUs();
#endif
}

static void doTCSCapture(bool black) {
#if USE_FREQ_MODE
  float r, g, b;
#else
  unsigned long r, g, b;
#endif
  readTCSRawOnce(r, g, b);
  if (black) {
    tcsHasBlack = true; r_blk = r; g_blk = g; b_blk = b;
    Serial.printf("Captured BLACK  R=%.0f  G=%.0f  B=%.0f\n", (double)r, (double)g, (double)b);
  } else {
    tcsHasWhite = true; r_wht = r; g_wht = g; b_wht = b;
    Serial.printf("Captured WHITE  R=%.0f  G=%.0f  B=%.0f\n", (double)r, (double)g, (double)b);
  }

  if (tcsHasBlack && tcsHasWhite) {
    Serial.println(F("\n=== Suggested values for sensors.cpp (top of file) ==="));
#if USE_FREQ_MODE
    Serial.printf("float RED_FREQ_DARK = %.0ff, RED_FREQ_BRIGHT = %.0ff;\n", (double)r_blk, (double)r_wht);
    Serial.printf("float GRN_FREQ_DARK = %.0ff, GRN_FREQ_BRIGHT = %.0ff;\n", (double)g_blk, (double)g_wht);
    Serial.printf("float BLU_FREQ_DARK = %.0ff, BLU_FREQ_BRIGHT = %.0ff;\n", (double)b_blk, (double)b_wht);
#else
    Serial.printf("int   RED_DARK_US   = %lu, RED_BRIGHT_US   = %lu;\n", r_blk, r_wht);
    Serial.printf("int   GRN_DARK_US   = %lu, GRN_BRIGHT_US   = %lu;\n", g_blk, g_wht);
    Serial.printf("int   BLU_DARK_US   = %lu, BLU_BRIGHT_US   = %lu;\n", b_blk, b_wht);
#endif
    Serial.println(F("======================================================\n"));
  }
}

static void printRawOnce() {
  float v = readPHVoltsAvg();
#if USE_FREQ_MODE
  float r, g, b; readTCSRawOnce(r, g, b);
  Serial.printf("RAW: pH volts=%.4f V | TCS(Hz) R=%.0f G=%.0f B=%.0f\n", (double)v, (double)r, (double)g, (double)b);
#else
  unsigned long r, g, b; readTCSRawOnce(r, g, b);
  Serial.printf("RAW: pH volts=%.4f V | TCS(us) R=%lu G=%lu B=%lu\n", (double)v, r, g, b);
#endif
}

static void handleCalConsole() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') continue;
    switch (c) {
      case 'h': case 'H': printHelp(); break;
      case 'A': doPHCapture('A'); break;
      case 'B': doPHCapture('B'); break;
      case 'k': case 'K': doTCSCapture(true);  break;  // black
      case 'w': case 'W': doTCSCapture(false); break;  // white
      case 'r': case 'R': printRawOnce();      break;
      default: Serial.println(F("(unknown) Type 'h' for help.")); break;
    }
  }
}
// ================= END CALIBRATION CONSOLE =================


// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  delay(200);
  LOGI("Aquarium Controller starting…");

  pins::initPins();
  initButton();
  sensors::init();

  Wire.begin(pins::LCD_SDA, pins::LCD_SCL);
  lcdAddr = probeI2C(0x27) ? 0x27 : (probeI2C(0x3F) ? 0x3F : 0x27);
  lcd = new LiquidCrystal_I2C(lcdAddr, 16, 2);
  lcd->init(); lcd->backlight(); lcd->clear();
  lcdPrint16(0, 0, "Aquarium Ctrl");
  lcdPrint16(0, 1, "Serial: 'h' help");
  delay(1200); lcd->clear();

  LOGI("Init done. LCD @ 0x%02X. Open Serial Monitor (115200), type 'h' for calibration help.", lcdAddr);
}

// ======================== LOOP ========================
void loop() {
  // Handle calibration console at any time
  handleCalConsole();
  updateButton(lcd);
  if (!systemEnabled) {
    // System OFF → skip all sensor reads & updates
    delay(200);
  return;
  }

  // Periodic sensor read + display
  static uint32_t last = 0;
  if (millis() - last < SENSOR_SAMPLE_PERIOD_MS) return;
  last = millis();

  sensors::Reading r = sensors::readAll();

  // Dominant color (by normalized channels)
  char dom = 'R';
  if (r.gN >= r.rN && r.gN >= r.bN) dom = 'G';
  else if (r.bN >= r.rN && r.bN >= r.gN) dom = 'B';

  // LCD updates
  char line1[17];
  if (!isnan(r.tempC)) snprintf(line1, sizeof(line1), "T:%4.1fC  pH:%4.2f", r.tempC, r.pH);
  else                 snprintf(line1, sizeof(line1), "T:----   pH:%4.2f", r.pH);
  lcdPrint16(0, 0, String(line1));

  char line2[17];
  snprintf(line2, sizeof(line2), "Lvl:%s Clr:%c", (r.levelWet ? "WET " : "DRY "), dom);
  lcdPrint16(0, 1, String(line2));

  LOGI("TempC=%.2f  pH=%.2f  Level=%s  RGBn=(%.2f,%.2f,%.2f) CHz=%.0f",
       r.tempC, r.pH, (r.levelWet ? "WET" : "DRY"), r.rN, r.gN, r.bN, r.colorHz);
}