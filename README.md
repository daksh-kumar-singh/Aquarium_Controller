# Aquarium Controller Firmware (ESP32)

Firmware for an ESP32-based smart aquarium monitoring subsystem.  
The controller reads **temperature**, **pH**, **water level**, and **color** data, then outputs live status on a **16×2 I²C LCD** while providing a **Serial Calibration Console** for accurate pH and TCS3200 color calibration.

Final integration into the main MCU system will be completed by  
**Taylor Louk (MCU Subsystem Owner).**

---

## Features

### Sensor Suite
- **DS18B20** temperature sensor  
- **pH probe** (ADC1 input, 2-point linear calibration)  
- **Float switch** for water level detection  
- **TCS3200** color sensor  
  - Frequency or pulse-width mode  
  - Median filtering (configurable)

### User Interface
- **16×2 I²C LCD** (auto-detects `0x27` → `0x3F`)  
- **Pushbutton** (GPIO13)  
  - Toggles system ON/OFF  
  - When OFF, all sensor reads pause  

### Serial Calibration Console (115200 baud)
Command | Description
--------|-------------
`h` | Help menu  
`A` | Capture pH calibration point A  
`B` | Capture pH calibration point B  
`k` | Capture TCS black reference  
`w` | Capture TCS white reference  
`r` | One-shot raw readings  

The console prints copy-paste constants for `config.h` and `sensors.cpp`.

---

## Repository Structure

```
Aquarium_Controller/
│
├── Aquarium_Controller.ino
├── config.h
├── pins.h
├── sensors.h
├── sensors.cpp
├── logging.h
│
└── examples/
     ├── temp_sensor/
     ├── ds18b20_button_test/
     ├── float_switch_test/
     └── color_sensor_LEDs/
```

---

## Hardware Wiring (matches `pins.h`)

| Component / Signal | ESP32 Pin | Notes |
|--------------------|-----------|-------|
| **DS18B20 data** | GPIO34 | Requires **4.7 kΩ pull-up** |
| **pH analog** | GPIO35 | ADC1 input-only |
| **Float switch** | GPIO2 | `INPUT_PULLUP` → HIGH=dry, LOW=wet |
| **TCS3200 S0 / S1** | GPIO19 / GPIO18 | Frequency scaling |
| **TCS3200 S2 / S3** | GPIO17 / GPIO16 | Filter selection |
| **TCS3200 OUT** | GPIO21 | Frequency input |
| **LCD SDA / SCL** | GPIO13 / GPIO12 | I²C |
| **Pushbutton** | GPIO13 | `INPUT_PULLUP`; connect to **GND** |

### Pushbutton Wiring (4-pin tact switch)
```
GPIO13 ----[switch]---- GND
```

---

## Build Instructions

1. Install **Arduino IDE 2.x**  
2. Install **ESP32 core** (Espressif)  
3. Install libraries:  
   - `OneWire`  
   - `DallasTemperature`  
   - `LiquidCrystal_I2C`  
4. Open `Aquarium_Controller.ino`  
5. Select **ESP32 Dev Module**  
6. Upload  

---

## Runtime Behavior

### On Boot
- Initializes sensors & LCD  
- Auto-detects I²C address (`0x27` → `0x3F`)  
- Displays splash screen  
- Begins periodic updates (`SENSOR_SAMPLE_PERIOD_MS`)  

### Button Toggle
- Press button (GPIO13 → GND)  
- Toggles **System: ON/OFF**  
- When OFF → sensors pause, LCD displays OFF message  

### LCD Output
```
T:24.8C  pH:7.05
Lvl:WET  Clr:R
```

---

## Calibration Instructions

### pH Calibration (Two-Point)
1. Insert probe into **buffer A** (e.g., pH 7)  
   → press `A`, enter known pH  
2. Insert into **buffer B** (e.g., pH 4 or 10)  
   → press `B`, enter known pH  
3. Console prints:
   ```c
   #define PH_SLOPE_M   <value>f
   #define PH_OFFSET_B  <value>f
   ```
4. Copy into `config.h`

---

### TCS3200 Color Calibration
1. Hold sensor over **black target** → press `k`  
2. Hold sensor over **white target** → press `w`  
3. Console prints recommended constants:
   ```c
   float RED_FREQ_DARK = ...;
   float RED_FREQ_BRIGHT = ...;
   // etc.
   ```
4. Replace values at top of `sensors.cpp`  

Supports:
- **Frequency mode** (`USE_FREQ_MODE = 1`)  
- **Pulse-width mode** (`USE_FREQ_MODE = 0`)  

---

## Configuration Options (`config.h`)

| Name | Description |
|------|-------------|
| `SENSOR_SAMPLE_PERIOD_MS` | Update period |
| `PH_SLOPE_M`, `PH_OFFSET_B` | Calibration constants |
| `USE_FREQ_MODE` | `1`=Hz mode, `0`=pulse-width |
| `TCS_MEDIAN_SAMPLES` | Odd number, sample count |
| `TCS_START_20_PERCENT` | Use 20% frequency scaling |

Float switch logic:  
`digitalRead(GPIO2) == HIGH` → **dry**  
Change logic in `readLevelWet()` if reversed.

---

## Troubleshooting

- **LCD blank:** Check SDA/SCL wiring (GPIO13/12), verify address.  
- **DS18B20 error:** Ensure pull-up resistor and wiring to GPIO34.  
- **Color unstable:** Recalibrate, shield from ambient light, increase median samples.  
- **Incorrect pH:** Re-run calibration; update `config.h`.  

---

## License
MIT

