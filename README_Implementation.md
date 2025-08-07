# ESP32-S3 Commercial Freezer Controller

This project implements a complete commercial freezer controller system for the **Waveshare ESP32-S3-Touch-LCD-4.3B Type B** development board.

## System Overview

The controller provides:
- Real-time temperature monitoring using DS18B20 sensors
- Intelligent cooling control with hysteresis
- Automated and manual defrost cycles
- Comprehensive alarm system
- Professional HMI with touch interface
- Fault detection and safe-mode operation
- Data logging capabilities

## Hardware Requirements

### Main Board
- **Waveshare ESP32-S3-Touch-LCD-4.3B Type B**
  - ESP32-S3-WROOM-1-N16R8 (16MB QIO Flash, 8MB Octal PSRAM)
  - 4.3" 800×480 IPS display with ST7262 controller
  - GT911 5-point capacitive touch
  - CH422G I/O expander for display control
  - PCF85063A Real-Time Clock

### External Components
- **PCF8574 I/O Expander** (address 0x20) for relay control
- **4× DS18B20 Temperature Sensors** (1-Wire)
  - Cabin sensor (primary control)
  - Evaporator sensor (defrost control)
  - Condenser sensor (monitoring)
  - Suction sensor (diagnostics)
- **Relay Module** with auxiliary feedback contacts
  - Compressor control relay
  - Evaporator fan relay
  - Hot gas defrost solenoid
  - Electric defrost heater (optional)
- **Buzzer** for audible alarms

## Architecture

### Dual-Core Task Design

**Core 0 (Protocol Core)** - UI Tasks:
- `lvgl_task` - LVGL rendering and animations (Priority 5)
- Touch processing and input handling

**Core 1 (Application Core)** - Control Tasks:
- `control_task` - Main temperature control logic (Priority 3)
- `sensor_task` - Temperature sensor readings (Priority 2)

### Memory Utilization

- **Internal SRAM (512KB)**: FreeRTOS kernel, task stacks, critical variables
- **External PSRAM (8MB)**: LVGL frame buffers, graphics memory, application heap
- **Flash (16MB)**: Firmware, assets, configuration storage

### Critical Initialization Sequence

⚠️ **MANDATORY ORDER** - The Type B board requires this exact sequence:

1. **I²C Bus Initialization** (GPIO 8/9)
2. **CH422G I/O Expander Setup** (address 0x71)
3. **Display Hardware Reset** via CH422G
4. **RGB Display Panel Initialization**
5. **Backlight Enable** via CH422G
6. **LVGL Graphics System Setup**

## Features

### Temperature Control
- Precision control with configurable setpoint and hysteresis
- Minimum compressor on/off times for equipment protection
- Backup cooling mode if cabin sensor fails
- Smart defrost scheduling based on time and demand

### Alarm System
- High/low temperature alarms with visual and audible indicators
- 20-minute silence function with automatic re-alarm
- Priority-based fault detection and display
- Emergency stop on critical faults

### User Interface
- Modern touchscreen HMI with smooth animations
- Temperature up/down controls
- Manual defrost activation (3-second hold)
- Service menu access (5-second hold in corner)
- Real-time status indicators

### Fault Detection
- Sensor open/short circuit detection
- Relay feedback verification
- I²C communication monitoring
- Latching fault system for critical issues

## Build Instructions

### Prerequisites
- PlatformIO Core or PlatformIO IDE
- ESP32 development environment

### Compilation
```bash
# Clone the repository
git clone <repository-url>
cd Temperature_control

# Build the project
pio run

# Upload to board
pio run -t upload

# Monitor serial output
pio device monitor
```

### Library Dependencies
The project uses specific locked versions of critical libraries:

- **ESP32_Display_Panel@1.0.0** (local) - ST7262 controller with CH422G support
- **ESP32_IO_Expander@1.0.1** (local) - CH422G I/O expander driver
- **LVGL@8.3.11** - Graphics library (locked version)
- **PCF8574** - External I/O expander for relays
- **DallasTemperature** - DS18B20 sensor library
- **OneWire** - 1-Wire bus protocol

⚠️ **Do not update the local libraries** - they contain Type B board specific code.

## Configuration

### Hardware Pin Assignments

#### Fixed Pins (DO NOT CHANGE)
```cpp
// I²C Bus
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// Display RGB Interface (28 pins)
// See config.h for complete pin mapping

// Touch Controller
#define TOUCH_IRQ_PIN 4
```

#### Application Pins (Configurable)
```cpp
// Temperature Sensors
#define DS18B20_PIN 33  // 1-Wire bus

// User I/O
#define BUZZER_PIN 34
#define LED_STATUS_PIN 35
```

### Control Parameters
```cpp
// Temperature Control
#define DEFAULT_SETPOINT_C -18.0f
#define TEMP_HYSTERESIS_C 2.0f
#define TEMP_ALARM_DIFFERENTIAL_C 5.0f

// Defrost Configuration
#define DEFROST_INTERVAL_HOURS 6
#define DEFROST_DURATION_MS (20 * 60 * 1000)
#define DEFROST_TERMINATION_TEMP_C 10.0f
```

## Service Menu

Access the service menu by pressing and holding the top-right corner of the screen for 5 seconds.

### Available Functions
- **Live Data**: Real-time sensor readings and system status
- **Settings**: Adjustable control parameters
- **Calibration**: Sensor offset adjustments
- **Diagnostics**: Fault history and system information

## Safety Features

### Critical Fault Handling
- **PCF8574 Communication Failure**: Emergency stop all outputs
- **Compressor Feedback Mismatch**: Compressor shutdown and fault latch
- **Cabin Sensor Failure**: Switch to backup cooling mode

### Backup Cooling Mode
When the cabin sensor fails, the system automatically switches to:
1. Evaporator temperature control (if available)
2. Timed cooling cycles (15 min on, 15 min off)

## Troubleshooting

### Black Screen on Boot
1. Check I²C connections (GPIO 8/9)
2. Verify CH422G at address 0x71: `i2cdetect -y 1`
3. Ensure proper power supply (5V, adequate current)
4. Check initialization sequence in serial output

### Display Artifacts or Flickering
1. Verify all 28 RGB data pins are connected correctly
2. Check PCLK frequency (25 MHz for this display)
3. Ensure adequate power supply for display

### Touch Not Responding
1. Verify GT911 interrupt on GPIO 4
2. Check touch controller I²C address (0x5D or 0x14)
3. Ensure proper ground connections

### Sensor Reading Issues
1. Check 1-Wire bus on GPIO 33 (changed from GPIO 15)
2. Verify 4.7kΩ pull-up resistor on data line
3. Check sensor power supply (3.3V or parasitic)
4. Use sensor discovery function in service menu

## Future Enhancements

### Planned Features
- **Wi-Fi Connectivity**: Remote monitoring via MQTT
- **OTA Updates**: Firmware updates over-the-air
- **Data Logging**: Historical temperature and performance data
- **Energy Monitoring**: Power consumption tracking
- **Predictive Maintenance**: Component lifecycle monitoring

### Expansion Possibilities
- **Modbus RTU**: Integration with building management systems
- **Additional Sensors**: Humidity, pressure, door switches
- **Multi-Zone Control**: Multiple temperature zones
- **Advanced Defrost**: Adaptive algorithms based on usage patterns

## Technical Notes

### Performance Characteristics
- **UI Refresh Rate**: 60+ FPS smooth animations
- **Control Loop**: 1 Hz precision temperature control
- **Sensor Reading**: 0.5 Hz with averaging
- **Memory Usage**: ~2MB PSRAM for graphics, 512KB SRAM for system

### Power Consumption
- **Typical Operation**: ~200mA @ 5V
- **Peak (Backlight + Processing)**: ~400mA @ 5V
- **Deep Sleep Mode**: Not implemented (continuous operation required)

## License

This project is developed for commercial freezer applications. Please review licensing terms before commercial deployment.

## Support

For technical support or customization requests, please refer to the detailed technical documentation in the `Reports.zip` file included in this repository.