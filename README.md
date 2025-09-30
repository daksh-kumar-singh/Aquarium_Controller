# Aquarium_Controller (ESP32)

Sensors & Testing subsystem firmware for a smart aquarium controller.

## Features (current)
- Reads DS18B20 temperature, pH via ADC, float switch (level), and TCS3200 color freq.
- Periodic sampling and Serial logging.
- Optional BLE notifications (toggle in `config.h`).

## Hardware pins
- DS18B20 data → GPIO 4 (4.7k pull-up to 3.3V)
- Float switch → GPIO 5 (INPUT_PULLUP)
- pH ADC → GPIO 34
- TCS3200 OUT → GPIO 14

## Build
1. Arduino IDE 2.x + ESP32 core (Espressif).
2. Libraries: OneWire, DallasTemperature, (optional) ESP32 BLE Arduino.
3. Open `Aquarium_Controller.ino` and upload to ESP32 Dev Module.

## Calibration
- pH: set `PH_SLOPE` and `PH_OFFSET` in `config.h` using 2-point buffers (pH 7 & 4/10).
- Temp: set `TEMP_OFFSET_C` after comparing to a reference thermometer.

## License
??