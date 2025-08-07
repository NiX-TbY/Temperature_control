# Quick Start Guide - ESP32-S3 Freezer Controller

## 🚀 Getting Started

### 1. Hardware Setup

#### Waveshare ESP32-S3-Touch-LCD-4.3B Type B
- Connect USB-C cable for programming and power
- Ensure board is Type B (check model number)

#### External Connections

**PCF8574 I/O Expander (Address 0x20)**
```
VCC  -> 3.3V
GND  -> GND
SDA  -> GPIO 8 (shared I²C)
SCL  -> GPIO 9 (shared I²C)
A0   -> GND (sets address 0x20)
A1   -> GND
A2   -> GND
```

**Temperature Sensors (DS18B20)**
```
VDD  -> 3.3V
GND  -> GND
DATA -> GPIO 33 (with 4.7kΩ pull-up to 3.3V)
```

**Relay Module**
```
PCF8574 P0 -> Compressor Relay Coil
PCF8574 P1 -> Evaporator Fan Relay Coil
PCF8574 P2 -> Hot Gas Defrost Solenoid
PCF8574 P3 -> Electric Defrost Heater (optional)
PCF8574 P4 <- Compressor Relay Aux Contact (NO to GND)
PCF8574 P5 <- Evaporator Fan Relay Aux Contact (NO to GND)
```

**Buzzer**
```
+    -> GPIO 34
-    -> GND
```

### 2. Software Setup

#### Install PlatformIO
```bash
pip install platformio
```

#### Build and Upload
```bash
# Clone repository
git clone <repo-url>
cd Temperature_control

# Build project
pio run

# Upload to board
pio run -t upload

# Monitor serial output
pio device monitor
```

### 3. First Boot

#### Expected Behavior
1. **Startup Phase** (30 seconds)
   - Display initializes with backlight
   - "Commercial Freezer Controller" appears
   - Sensors discovery and calibration
   - System enters normal operation

#### Troubleshooting First Boot

**Black Screen**
```bash
# Check I²C devices
i2cdetect -y 1
# Should show devices at 0x51 (RTC), 0x5D (Touch), 0x71 (CH422G)
```

**Serial Debug Output**
```
=== ESP32-S3 Commercial Freezer Controller ===
Firmware Version: 1.0.0
Free Heap: XXXXX bytes
Free PSRAM: XXXXX bytes
Initializing hardware...
CH422G I/O expander initialized successfully
Display panel initialized successfully
...
```

### 4. Basic Operation

#### Main Screen Layout
```
┌─────────────────┬──────────────────────────────┐
│  Setpoint       │                              │
│  -18°C          │         -18.2°C              │
│                 │      (Large Display)         │
├─────────────────┤                              │
│      ▲UP       │                              │
├─────────────────┤                              │
│     ▼DOWN      │                              │
├─────────────────┤                              │
│  ❄ DEFROST     │                              │
└─────────────────┴──────────────────────────────┘
```

#### Controls
- **Temperature Up/Down**: Single tap to adjust setpoint (±1°C)
- **Manual Defrost**: Hold DEFROST button for 3 seconds
- **Service Menu**: Hold top-right corner for 5 seconds

### 5. Configuration

#### Temperature Settings
```cpp
// In src/config.h - modify these values:
#define DEFAULT_SETPOINT_C -18.0f      // Default temperature
#define TEMP_HYSTERESIS_C 2.0f         // Control deadband
#define TEMP_ALARM_DIFFERENTIAL_C 5.0f // Alarm threshold
```

#### Defrost Settings
```cpp
#define DEFROST_INTERVAL_HOURS 6        // Auto defrost every 6 hours
#define DEFROST_DURATION_MS (20*60*1000) // Max 20 minutes
#define DEFROST_TERMINATION_TEMP_C 10.0f // Stop when evap reaches 10°C
```

### 6. Service Menu Functions

#### Live Data Tab
- Real-time temperatures from all 4 sensors
- Compressor/fan status
- System uptime and performance metrics

#### Settings Tab
- Setpoint adjustment (-30°C to 0°C)
- Hysteresis configuration
- Defrost interval and duration
- Alarm thresholds

#### Calibration Tab
- Individual sensor offset adjustment (±10°C)
- Factory reset option
- Sensor discovery and replacement

#### Diagnostics Tab
- Active fault codes
- Fault history log
- System health status
- Clear latched faults

### 7. Alarm Handling

#### High Temperature Alarm
1. **Visual**: Temperature display flashes red
2. **Audible**: Buzzer sounds continuously
3. **Action**: Press "SILENCE" button for 20-minute silence
4. **Auto-Clear**: Alarm clears when temperature returns to normal

#### Fault Conditions
- **Sensor Faults**: Display shows fault name instead of setpoint
- **Critical Faults**: System enters safe mode, all outputs off
- **Communication Faults**: Emergency stop and audible alarm

### 8. Common Issues & Solutions

#### Sensor Not Reading
```cpp
// Check 1-Wire bus in service menu
// Expected: 4 sensors discovered
// If less: Check wiring, power, pull-up resistor
```

#### Compressor Won't Start
1. Check relay feedback - should match command state
2. Verify minimum off-time (3 minutes) has elapsed
3. Check for active fault codes
4. Ensure temperature is above setpoint + hysteresis

#### Display Touch Not Working
1. Check GT911 interrupt line (GPIO 4)
2. Verify I²C communication to touch controller
3. Restart system if touch controller stuck

#### Relay Feedback Mismatch
```
Fault: "COMPRESSOR FEEDBACK FAIL"
Cause: Relay auxiliary contact not matching command
Solution: Check relay wiring, replace faulty relay
```

### 9. Performance Monitoring

#### Memory Usage
```
# Check via serial monitor:
Total heap used: XXXXX bytes
PSRAM used: XXXXX bytes
```

#### Task Performance
```
# Monitor via service menu or serial:
LVGL task: Running on Core 0
Control task: Running on Core 1
Sensor task: Running on Core 1
```

### 10. Customization Examples

#### Change Temperature Units to Fahrenheit
```cpp
// In hmi_manager.cpp:
lv_label_set_text_fmt(label_actual_temp, "%.1f°F", celsius_to_fahrenheit(actual_temp));
```

#### Add Custom Alarm Delay
```cpp
// In config.h:
#define HIGH_TEMP_ALARM_DELAY_MS (10 * 60 * 1000)  // 10 minutes
```

#### Modify Compressor Protection Times
```cpp
// In control_logic.h:
static constexpr uint32_t MIN_COMPRESSOR_ON_TIME_MS = 300000;   // 5 minutes
static constexpr uint32_t MIN_COMPRESSOR_OFF_TIME_MS = 240000;  // 4 minutes
```

## 🔧 Advanced Configuration

For detailed technical documentation, hardware specifications, and advanced configuration options, see:
- `README_Implementation.md` - Complete technical documentation
- `Reports.zip` - Original hardware analysis and specifications
- Source code comments for detailed API documentation

## 📞 Support

- Check serial monitor output for diagnostic information
- Use service menu diagnostics tab for system health
- Refer to fault code documentation for troubleshooting
- Monitor task performance and memory usage