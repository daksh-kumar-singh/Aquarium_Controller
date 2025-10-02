/*
  ds18b20_button_test.ino
  DS18B20 with button ON/OFF control on ESP32
  - Reads a single DS18B20 sensor
  - Prints °C and °F when ON
  - Button toggles ON/OFF state
*/

#include <OneWire.h>
#include <DallasTemperature.h>

// -------------------- Pins --------------------
#define ONE_WIRE_BUS 26   // DS18B20 data line (with 4.7k pull-up to 3.3V)
#define BUTTON_PIN   12   // Pushbutton to toggle ON/OFF (use GPIO12 or any digital)

// -------------------- Objects -----------------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// -------------------- State -------------------
bool sensorOn = false;         // start OFF
bool lastButtonState = HIGH;   // assume pull-up wiring

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("DS18B20 Temperature Test (Button ON/OFF)"));

  // Enable internal pull-up on data line (external 4.7k still required)
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);

  // Button with internal pull-up: press → LOW
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  sensors.begin();
  if (sensors.getDeviceCount() == 0) {
    Serial.println(F("⚠️  No DS18B20 detected. Check wiring + pull-up."));
  }
}

void loop() {
  // ---- Button handling ----
  bool buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Button just pressed → toggle ON/OFF
    sensorOn = !sensorOn;
    if (sensorOn) Serial.println(F("Sensor turned ON"));
    else Serial.println(F("Sensor turned OFF"));
    delay(250); // debounce
  }
  lastButtonState = buttonState;

  // ---- Sensor ON ----
  if (sensorOn) {
    sensors.requestTemperatures();
    float tC = sensors.getTempCByIndex(0);

    if (tC == DEVICE_DISCONNECTED_C) {
      Serial.println(F("Error: DS18B20 not detected!"));
    } else {
      float tF = DallasTemperature::toFahrenheit(tC);
      Serial.print(F("Temperature: "));
      Serial.print(tC, 2);
      Serial.print(F(" °C / "));
      Serial.print(tF, 2);
      Serial.println(F(" °F"));
    }
  } else {
    Serial.println(F("Sensor OFF"));
  }

  delay(1000);
}
