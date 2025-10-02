/*
  float_switch_test.ino
  ESP32 test for Anndason plastic float switch (reed switch).
  - Wire one lead to GND, the other to GPIO27.
  - Uses internal pull-up (INPUT_PULLUP), so pressed/closed -> LOW.
  - Debounced edge detection; prints on state changes and periodic status.
*/

#include <Arduino.h>

// -------------------- Settings --------------------
#define FLOAT_PIN           27      // GPIO27 for float switch
#define INVERT_LOGIC        true  // set to true if your "wet/dry" prints feel flipped
#define DEBOUNCE_MS         40     // debounce time
#define HEARTBEAT_MS        1000   // periodic status print

// -------------------- State -----------------------
static int lastStable = HIGH;            // INPUT_PULLUP idle = HIGH
static int lastReading = HIGH;
static unsigned long lastChangeMs = 0;
static unsigned long lastHeartbeatMs = 0;

// Helper: translate raw pin level to wet/dry considering INVERT_LOGIC
static bool isWetFromLevel(int level) {
  // With INPUT_PULLUP: CLOSED -> LOW, OPEN -> HIGH
  bool wet = (level == LOW);             // default: LOW means "wet/closed"
  if (INVERT_LOGIC) wet = !wet;          // flip if your float orientation is opposite
  return wet;
}

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println(F("\nFloat Switch Test (GPIO27, INPUT_PULLUP)"));
  Serial.println(F("Wire one lead to GND, the other to GPIO27."));

  pinMode(FLOAT_PIN, INPUT_PULLUP);

  // Initialize states
  int initLevel = digitalRead(FLOAT_PIN);
  lastStable = initLevel;
  lastReading = initLevel;
  lastChangeMs = millis();

  Serial.printf("Initial state: %s\n", isWetFromLevel(initLevel) ? "WET/CLOSED" : "DRY/OPEN");
}

void loop() {
  unsigned long now = millis();

  // ----- Debounce -----
  int reading = digitalRead(FLOAT_PIN);
  if (reading != lastReading) {
    lastReading = reading;
    lastChangeMs = now; // reset debounce timer on any change
  }

  if ((now - lastChangeMs) > DEBOUNCE_MS) {
    // reading has been stable long enough to consider it the new state
    if (reading != lastStable) {
      lastStable = reading;
      bool wet = isWetFromLevel(lastStable);
      Serial.printf("State change: %s (level=%s)\n",
                    wet ? "WET/CLOSED" : "DRY/OPEN",
                    (lastStable == LOW ? "LOW" : "HIGH"));
    }
  }

  // ----- Heartbeat (periodic status) -----
  if (now - lastHeartbeatMs >= HEARTBEAT_MS) {
    lastHeartbeatMs = now;
    bool wet = isWetFromLevel(lastStable);
    Serial.printf("Status: %s\n", wet ? "WET/CLOSED" : "DRY/OPEN");
  }
}
