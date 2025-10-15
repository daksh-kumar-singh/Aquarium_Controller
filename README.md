# Aquarium_Controller (ESP32)

Sensors & Testing subsystem firmware for a smart aquarium controller.  
Reads temperature (DS18B20), pH (ADC), water level (float switch), and color (TCS3200), and shows a compact status on a 16×2 I²C LCD. Includes a Serial **Calibration Console** for pH and color, and a **pushbutton toggle** (GPIO13) to turn the whole subsystem ON/OFF.

---

## Features
- **Sensor suite**
  - **DS18B20** digital temperature (OneWire)
  - **pH probe** via ADC input (linear 2-point calibration)
  - **Float switch** (debounced in example, simple read in main)
  - **TCS3200** color sensor (frequency or pulse-width; median filtering)
- **UI**
  - 16×2 **I²C LCD** (auto-probes `0x27`, then `0x3F`)
  - **Pushbutton toggle** (GPIO13) to enable/disable all sensor reads & LCD updates
- **Calibration Console** (Serial @ 115200)
  - `h` help · `A`/`B` capture pH points · `k` capture **black** · `w` capture **white** · `r` print raw snapshot

---

## Hardware Wiring

| Component / Signal                  | ESP32 Pin | Notes |
|------------------------------------|-----------|-------|
| **DS18B20** data                   | **GPIO26** | OneWire; **4.7 kΩ pull-up** to 3.3 V required |
| **pH sensor** analog out           | **GPIO35** | ADC1 (input-only) |
| **Float switch**                   | **GPIO5**  | `INPUT_PULLUP`. **Main firmware currently treats HIGH = wet/closed.** Invert in code if needed. |
| **TCS3200** S0 / S1                | **GPIO16 / GPIO17** | Frequency scaling / power gating via S0/S1 |
| **TCS3200** S2 / S3                | **GPIO18 / GPIO19** | Color filter select |
| **TCS3200** OUT                    | **GPIO34** | Frequency output (input-only OK) |
| **LCD 1602 I²C** SDA / SCL         | **GPIO21 / GPIO22** | Uses `LiquidCrystal_I2C` (auto-detects `0x27` → `0x3F`) |
| **Pushbutton** (toggle ON/OFF)     | **GPIO13** | Wire to **GND** on press; pin configured `INPUT_PULLUP` |

**Pushbutton wiring (4-pin tact switch):** Use **one pair** of the switch (the two pins that are shorted across the narrow side). Connect **GPIO13** to one leg of that pair, the **diagonally opposite leg** to **GND**. Leave the other pair unconnected.

---

## Build

1. Install **Arduino IDE 2.x** and the **ESP32 core** (Espressif).
2. Install libraries (Library Manager):
   - **OneWire**
   - **DallasTemperature**
   - **LiquidCrystal_I2C**
3. Open `Aquarium_Controller.ino` and upload to **ESP32 Dev Module**.

---

## Run & Use

1. Open **Serial Monitor** at **115200**.  
2. LCD shows a splash, then live status (every `SENSOR_SAMPLE_PERIOD_MS`, default **1000 ms**).  
3. **Toggle system ON/OFF** with the pushbutton (GPIO13). LCD shows “System: ON/OFF” and sensors pause when OFF.  
4. **Calibration Console** commands (type in Serial at any time):
   - `h` — show help
   - `A` — capture **pH point A** (it will ask you to enter the known pH)
   - `B` — capture **pH point B**
   - `k` — capture **TCS BLACK** (hold at black target)
   - `w` — capture **TCS WHITE** (hold at white target)
   - `r` — print one-shot raw (pH volts + TCS raw)

---

## Calibration

### pH (2-point)
1. Place probe in **buffer 1** (e.g., pH 7). Press `A`, then enter the known pH in Serial.
2. Rinse, place in **buffer 2** (e.g., pH 4 or 10). Press `B`, enter known pH.
3. The console prints suggested lines:
   ```c
   #define PH_SLOPE_M   <value>f
   #define PH_OFFSET_B  <value>f
   ### Paste them into config.h and upload.

---

## Color (TCS3200)
1. Hold at **black** target → press `k`.
2. Hold at **white** target → press `w`.
3. Console prints suggested values for **sensors.cpp** (top of file) as either `*_FREQ_*` (Hz) or `*_US` (µs) depending on mode. Replace the existing calibration constants and upload.

---

## Configuration (`config.h`)
- **SENSOR_SAMPLE_PERIOD_MS** — sample/refresh period (default: **1000 ms**)
- **pH mapping:** `PH_SLOPE_M`, `PH_OFFSET_B` (set via calibration)
- **TCS3200 options:**
  - `USE_FREQ_MODE` (1 = Hz using HIGH+LOW; 0 = µs using HIGH pulse)
  - `TCS_MEDIAN_SAMPLES` (odd number; default 7)
  - `TCS_START_20_PERCENT` (initial scale 20% vs 100%)

> **Note on float switch:** The main firmware currently interprets `digitalRead(GPIO5) == HIGH` as **wet/closed**.  
> If your hardware yields the opposite, invert this logic in `sensors.cpp::readLevelWet()`.

---

## Example Test Sketches (`examples/`)
- `temp_sensor/` — DS18B20 continuous °C/°F
- `ds18b20_button_test/` — DS18B20 with ON/OFF toggle on **GPIO12**
- `color_sensor_LEDs/` — Standalone TCS3200 with ON/OFF and calibration helpers
- `float_switch_test/` — Debounced float switch test (**GPIO27** by default); set `INVERT_LOGIC` as needed

These sketches help verify wiring and perform calibration before running the integrated firmware.

---

## Troubleshooting
- **LCD shows nothing:** Confirm SDA/SCL (21/22), address `0x27`/`0x3F`, and `LiquidCrystal_I2C` installed.
- **DS18B20 reads `DEVICE_DISCONNECTED_C`:** Check **4.7 kΩ pull-up** from data to 3.3 V and wiring to **GPIO26**.
- **TCS3200 values noisy/low:** Use `TCS_START_20_PERCENT` vs 100%, shield from ambient light, rerun calibration, and update constants.
- **Float switch reversed:** Flip logic in `readLevelWet()` or re-orient the float.

---

## License
MIT