# Commercial Freezer Controller - Build Instructions

## Project Overview
This is a complete firmware implementation for a Commercial Freezer Controller using the **Waveshare ESP32-S3-Touch-LCD-4.3B** development board. The project includes a sophisticated HMI (Human-Machine Interface), temperature monitoring, relay control, and fault detection systems.

## Hardware Requirements
- **Waveshare ESP32-S3-Touch-LCD-4.3B** development board
- **PCF8574** I2C I/O expander for relay control
- **DS18B20** temperature sensors (up to 4 sensors)
- Relay modules for compressor, fan, and defrost control
- Buzzer for audible alarms

## Software Requirements
- **PlatformIO** (recommended) or Arduino IDE
- **ESP32 Arduino Core** v2.0.x or later
- Libraries (automatically managed by PlatformIO):
  - LVGL 8.3.11
  - PCF8574 library
  - OneWire library
  - DallasTemperature library

## Build Instructions

### Method 1: PlatformIO (Recommended)

1. **Install PlatformIO**
   ```bash
   # Install PlatformIO Core
   pip install platformio
   ```

2. **Clone/Navigate to Project Directory**
   ```bash
   cd Temperature_control
   ```

3. **Install Dependencies and Build**
   ```bash
   # Install all dependencies automatically
   pio lib install
   
   # Build the project
   pio run
   
   # Upload to board (connect via USB)
   pio run --target upload
   
   # Monitor serial output
   pio device monitor
   ```

### Method 2: Arduino IDE

1. **Install Arduino IDE** and ESP32 board support
2. **Install Required Libraries** via Library Manager:
   - LVGL by kisvegabor (version 8.3.11)
   - PCF8574 library by Rob Tillaart
   - OneWire by Paul Stoffregen  
   - DallasTemperature by Miles Burton

3. **Open Project**
   - Open `src/main.cpp` in Arduino IDE
   - Select board: "ESP32S3 Dev Module"
   - Configure board settings:
     - Flash Size: 16MB
     - PSRAM: OPI PSRAM
     - Partition Scheme: Default

4. **Compile and Upload**

## Configuration

### Hardware Pin Configuration
The pin assignments are defined in `include/config.h`:

```cpp
// I2C Bus
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// Temperature Sensors  
#define DS18B20_PIN 33

// Buzzer
#define BUZZER_PIN 34

// I2C Addresses
#define IOEXP_I2C_ADDR 0x71      // CH422G (onboard)
#define PCF8574_I2C_ADDR 0x20    // PCF8574 (external)
```

### Application Parameters
Control parameters can be adjusted in `include/config.h`:

```cpp
#define DEFAULT_SETPOINT_C -18.0f              // Default target temperature
#define TEMP_HYSTERESIS_C 2.0f                 // Temperature control deadband
#define HIGH_TEMP_ALARM_DIFFERENTIAL_C 5.0f    // High temp alarm threshold
#define DEFROST_INTERVAL_HOURS 6               // Automatic defrost interval
#define DEFROST_DURATION_MIN 20                // Defrost cycle duration
```

## System Architecture

### Task Structure
The firmware uses a dual-core FreeRTOS architecture:

- **Core 0** (Protocol Core): High-priority UI tasks
  - `lvgl_task`: LVGL graphics rendering and input handling
  
- **Core 1** (Application Core): Control and sensor tasks  
  - `control_logic_task`: Main control algorithm and fault detection
  - `sensor_task`: Temperature sensor reading and processing

### Memory Management
- **PSRAM**: Used for LVGL display buffers and graphics objects
- **Internal RAM**: Reserved for FreeRTOS kernel and critical operations
- **Flash**: Stores application code and static assets

## Features

### HMI (Human-Machine Interface)
- **800x480 color touchscreen** with high-visibility display
- **Real-time temperature display** with large, easy-to-read fonts
- **Setpoint adjustment** via touch buttons
- **Visual and audible alarms** with silence functionality  
- **Defrost control** with manual trigger and status indication
- **Service menu** with advanced configuration options

### Control System
- **PID-style temperature control** with configurable hysteresis
- **Automatic defrost cycles** based on time or temperature
- **Manual defrost trigger** via long-press interface
- **Relay feedback monitoring** for fault detection
- **Multi-sensor support** (cabin, evaporator, condenser, suction)

### Fault Detection
- **Sensor failure detection** (open/short circuit)
- **I/O expander communication monitoring**  
- **Relay feedback verification**
- **Prioritized fault handling** with operator notifications
- **Automatic system shutdown** on critical faults

### Safety Features
- **Fail-safe operation** - system shuts down on communication errors
- **Alarm silence with timeout** - prevents nuisance alarms
- **Service access control** - hidden menu for technician use
- **Watchdog protection** via FreeRTOS task monitoring

## Troubleshooting

### Build Issues

**LVGL Compilation Errors:**
- Ensure `lv_conf.h` is in the include path
- Verify LVGL version is exactly 8.3.11
- Check that `LV_CONF_INCLUDE_SIMPLE` is defined

**Library Not Found:**
```bash
# Reinstall libraries
pio lib uninstall --all
pio lib install
```

**Memory Issues:**
- Ensure PSRAM is enabled in platformio.ini
- Check that `board_build.psram_mode = octal` is set
- Verify MALLOC_CAP_SPIRAM flag usage

### Runtime Issues

**Black Screen:**
1. Check I2C bus initialization (GPIO 8/9)
2. Verify CH422G communication at address 0x71
3. Confirm display reset sequence execution
4. Check backlight enable signal

**No Temperature Readings:**
1. Verify DS18B20 wiring on GPIO 33
2. Check 4.7kΩ pull-up resistor on data line
3. Monitor serial output for sensor detection messages

**Relay Control Issues:**
1. Confirm PCF8574 I2C address (default 0x20)
2. Check I2C bus for address conflicts
3. Verify relay wiring and power supply

## Serial Monitor Output
The system provides detailed boot and operation messages:

```
===========================================
Commercial Freezer Controller Booting...
ESP32-S3-Touch-LCD-4.3B Platform
===========================================
1. Initializing I2C Bus...
   I2C Bus initialized successfully
2. Initializing CH422G I/O Expander...
   CH422G initialized successfully
...
===========================================
SYSTEM READY - Commercial Freezer Controller
===========================================
```

## Customization

### Adding New Sensors
1. Modify `sensor_task()` in `main.cpp`
2. Update sensor array size in `config.h`
3. Add display elements in `hmi_manager.cpp`

### Modifying Control Algorithm
1. Edit `control_logic_task()` in `main.cpp`
2. Adjust parameters in `config.h`
3. Test thoroughly with temperature simulation

### UI Customization
1. Modify layouts in `hmi_manager.cpp`
2. Add new screens and widgets as needed
3. Update animation and style definitions

## Support and Documentation

For detailed hardware information, refer to:
- `README.md` - Comprehensive technical analysis
- `Hardware interface.pdf` - Detailed hardware specifications  
- `HMI Design.PDF` - User interface requirements
- `Logic References and parameter References.pdf` - Control algorithms

## License and Credits

This firmware is based on the comprehensive technical analysis and requirements documentation provided in the project repository. It implements a production-ready commercial freezer controller with modern HMI and robust fault detection capabilities.