# Aquarium_Controller (ESP32)

**NOTE:** Code in 'examples' folder was used for primary individual testing, the same have since been integrated to work together at the same time. 
Final integration with the main MCU system will be handled by **Taylor Louk (MCU Subsystem Owner).**

---

## Overview
This firmware handles **sensor reading, calibration, and LCD output** for a smart aquarium controller subsystem.  
It reads **temperature (DS18B20)**, **pH (ADC)**, **water level (float switch)**, and **color (TCS3200)**, while displaying live status on a **16×2 I²C LCD**.  
A **pushbutton toggle** (GPIO13) enables or disables all sensors, and a **Serial Calibration Console** provides an interface for pH and color calibration.

---

## Features
- **Sensor Suite**
  - **DS18B20** digital temperature (OneWire)
  - **pH probe** via ADC input (2-point linear calibration)
  - **Float switch** for water level
  - **TCS3200** color sensor (frequency or pulse-width; median filtering)
- **User Interface**
  - **16×2 I²C LCD** (auto-detects `0x27` → `0x3F`)
  - **Pushbutton** (GPIO13) toggle for ON/OFF system control
- **Calibration Console** (via Serial @ 115200)
  - Commands:  
    `h` help · `A` / `B` = pH calibration points ·  
    `k` = TCS black capture · `w` = TCS white capture · `r` = raw readout snapshot

---

## Hardware Wiring

| Component / Signal          | ESP32 Pin | Notes |
|-----------------------------|-----------|-------|
| **DS18B20** (data)          | GPIO26 | OneWire; requires **4.7 kΩ pull-up** to 3.3 V |
| **pH sensor** (analog out)  | GPIO35 | ADC1 (input-only) |
| **Float switch**            | GPIO5  | `INPUT_PULLUP`. Interpreted as **HIGH = wet/closed** (invert in code if needed). |
| **TCS3200** S0 / S1         | GPIO16 / GPIO17 | Frequency scaling |
| **TCS3200** S2 / S3         | GPIO18 / GPIO19 | Color filter selection |
| **TCS3200** OUT             | GPIO34 | Frequency output (input-only OK) |
| **LCD 1602 I²C** SDA / SCL  | GPIO21 / GPIO22 | Uses `LiquidCrystal_I2C` (auto-detects `0x27` → `0x3F`) |
| **Pushbutton**              | GPIO13 | Connect to **GND** on press; configured as `INPUT_PULLUP` |

**Pushbutton wiring (4-pin tact switch):**  
Use **one pair** of pins across the narrow side. Connect **GPIO13** to one leg and the **diagonally opposite leg** to **GND**. Leave the other pair unconnected.

---

## Build Instructions

1. Install **Arduino IDE 2.x** and **ESP32 core** (Espressif).
2. Install required libraries:
   - `OneWire`
   - `DallasTemperature`
   - `LiquidCrystal_I2C`
3. Open `Aquarium_Controller.ino` and upload to **ESP32 Dev Module**.

---

## Running the Firmware

1. Open the **Serial Monitor** at **115200 baud**.  
2. The LCD shows a splash screen, then updates live sensor values every `SENSOR_SAMPLE_PERIOD_MS` (default = 1000 ms).  
3. Press the **pushbutton** to toggle the system ON/OFF.  
   - LCD updates to “System: ON/OFF”  
   - When OFF, all sensors pause.  
4. Access the **Calibration Console** anytime through Serial:
   - `h` — help menu  
   - `A` — record pH point A (prompted for known pH)  
   - `B` — record pH point B  
   - `k` — capture TCS black reference  
   - `w` — capture TCS white reference  
   - `r` — print one-shot raw readings (pH voltage + TCS values)

---

## Calibration

### pH (2-Point Calibration)
1. Immerse the probe in **buffer 1** (e.g., pH 7). Press `A`, enter known pH.  
2. Rinse and place in **buffer 2** (e.g., pH 4 or 10). Press `B`, enter known pH.  
3. Console outputs:
   ```c
   #define PH_SLOPE_M   <value>f
   #define PH_OFFSET_B  <value>f
4. Paste these lines into `config.h` and re-upload.

---

## Color (TCS3200)

1. Hold sensor over a **black surface** → press `k`.  
2. Hold over a **white surface** → press `w`.  
3. Console prints suggested values for `sensors.cpp` (top of file) as `*_FREQ_*` (Hz) or `*_US` (µs).  
4. Replace the corresponding calibration constants and re-upload.

---

## Configuration (`config.h`)

- **SENSOR_SAMPLE_PERIOD_MS** — sample/refresh rate (default **1000 ms**)  
- **pH mapping:** `PH_SLOPE_M`, `PH_OFFSET_B`  
- **TCS3200 options:**  
  - `USE_FREQ_MODE` (1 = Hz via HIGH+LOW, 0 = µs via HIGH pulse)  
  - `TCS_MEDIAN_SAMPLES` (odd number, default 7)  
  - `TCS_START_20_PERCENT` (start scale 20% vs 100%)  

> **Note:**  
> The firmware treats `digitalRead(GPIO5) == HIGH` as **wet/closed**.  
> If reversed, modify logic in `sensors.cpp::readLevelWet()`.

---

## Example Test Sketches (`examples/`)

- `temp_sensor/` — DS18B20 continuous °C/°F readings  
- `ds18b20_button_test/` — DS18B20 with button ON/OFF toggle (GPIO12)  
- `color_sensor_LEDs/` — Standalone TCS3200 with ON/OFF control and calibration  
- `float_switch_test/` — Debounced float switch test (GPIO27 default); set `INVERT_LOGIC` if needed  

These sketches validate wiring and calibration before full integration.

---

## Troubleshooting

- **LCD blank:** Check SDA/SCL (21/22), I²C address (0x27 or 0x3F), and confirm that `LiquidCrystal_I2C` is installed.  
- **DS18B20 returns `DEVICE_DISCONNECTED_C`:** Verify **4.7 kΩ pull-up** from data to 3.3 V and wiring to **GPIO26**.  
- **TCS3200 readings noisy/low:** Try `TCS_START_20_PERCENT`, shield from ambient light, and recalibrate constants.  
- **Float switch reversed:** Invert logic in `readLevelWet()` or flip float orientation.

---

## License

MIT
