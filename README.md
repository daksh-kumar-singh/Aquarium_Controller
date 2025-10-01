# Aquarium_Controller (ESP32)

Sensors & Testing subsystem firmware for a smart aquarium controller.

## Features (current)
- Reads:
  - **DS18B20** temperature (OneWire)
  - **pH probe** via ADC input
  - **Float switch** for water level
  - **TCS3200** color sensor (frequency-based)
- Periodic sampling and Serial logging at configurable intervals.
- Modular design for easy future Wi-Fi integration (BLE has been removed).

## Hardware pins
- DS18B20 data → **GPIO 4** (with 4.7k pull-up to 3.3V)
- Float switch → **GPIO 5** (INPUT_PULLUP, LOW = wet/closed)
- pH analog in → **GPIO 35** (ADC1)
- TCS3200:
  - S0 → **GPIO 16**
  - S1 → **GPIO 17**
  - S2 → **GPIO 18**
  - S3 → **GPIO 19**
  - OUT → **GPIO 34** (input-only)

## Build
1. Install **Arduino IDE 2.x** with the **ESP32 core** (Espressif).
2. Install libraries:
   - [OneWire](https://www.arduino.cc/reference/en/libraries/onewire/)
   - [DallasTemperature](https://www.arduino.cc/reference/en/libraries/dallastemperature/)
3. Open `Aquarium_Controller.ino` and upload to **ESP32 Dev Module**.

## Calibration
- **pH**:  
  Perform 2-point calibration using buffer solutions (e.g., pH 7 & 4 or 10).  
  Update the slope (`PH_SLOPE_M`) and offset (`PH_OFFSET_B`) in `config.h`.
- **Temperature**:  
  If necessary, apply an offset in software by comparing DS18B20 to a reference thermometer.
- **Color (TCS3200)**:  
  Log raw black/white values, then update `*_DARK` and `*_BRIGHT` constants in `config.h`.  
  Normalized RGB values (`rN, gN, bN`) can be used for color detection independent of brightness.

## License
MIT