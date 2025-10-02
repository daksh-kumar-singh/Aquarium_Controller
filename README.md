# Aquarium_Controller (ESP32)

Sensors & Testing subsystem firmware for a smart aquarium controller.

## Features (current)
- Reads:
  - **DS18B20** digital temperature sensor (OneWire)
  - **pH probe** via ADC input
  - **Float switch** for water level
  - **TCS3200** color sensor (frequency-based)
- Periodic sampling and Serial logging at configurable intervals.
- Modular design for easy future Wi-Fi integration (BLE has been removed).

## Hardware Wiring

| Component        | ESP32 Pin | Notes |
|------------------|-----------|-------|
| DS18B20 (data)   | GPIO26    | Requires **4.7kΩ pull-up** to 3.3V |
| Float Switch     | GPIO5     | `INPUT_PULLUP`, reads **LOW when wet/closed** |
| pH Sensor (ADC)  | GPIO35    | Analog input (ADC1) |
| TCS3200 – S0     | GPIO16    | Frequency scaling |
| TCS3200 – S1     | GPIO17    | Frequency scaling |
| TCS3200 – S2     | GPIO18    | Color filter select |
| TCS3200 – S3     | GPIO19    | Color filter select |
| TCS3200 – OUT    | GPIO34    | Input-only pin |
| Pushbutton (ON/OFF for examples) | GPIO12 | `INPUT_PULLUP`, press → LOW |

## Example Test Sketches
The `examples/` folder contains standalone bring-up tests for each sensor:
- `temp_sensor/` → Simple DS18B20 test sketch (continuous °C/°F readings).  
- `ds18b20_button_test/` → DS18B20 with button ON/OFF toggle on GPIO12.  
- `color_sensor_LEDs/` → TCS3200 color sensor with ON/OFF control and calibration.  

These sketches are designed for wiring verification and calibration before integrating sensors into the main firmware.

## Build
1. Install **Arduino IDE 2.x** with the **ESP32 core** (Espressif).  
2. Install libraries:  
   - [OneWire](https://www.arduino.cc/reference/en/libraries/onewire/)  
   - [DallasTemperature](https://www.arduino.cc/reference/en/libraries/dallastemperature/)  
3. Open `Aquarium_Controller.ino` and upload to **ESP32 Dev Module**.  

## Calibration
- **pH**: Perform 2-point calibration using buffer solutions (pH 7 & 4/10).  
  Update slope (`PH_SLOPE_M`) and offset (`PH_OFFSET_B`) in `config.h`.  
- **Temperature**: If necessary, apply an offset by comparing DS18B20 to a reference thermometer.  
- **Color (TCS3200)**: Log raw black/white values, then update `*_DARK` and `*_BRIGHT` constants in `config.h`.  

## License
MIT
