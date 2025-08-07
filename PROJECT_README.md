# Temperature Control - ESP32-S3 Commercial Freezer Controller

## 🌡️ Project Overview

This repository contains a complete firmware implementation for a **Commercial Freezer Controller** using the **Waveshare ESP32-S3-Touch-LCD-4.3B** development board. The project transforms comprehensive technical documentation into a fully functional, production-ready embedded system.

### 🚀 Key Features

- **📱 Modern HMI**: 800x480 color touchscreen with LVGL graphics
- **🌡️ Multi-Sensor**: Support for up to 4 DS18B20 temperature sensors  
- **⚡ Dual-Core**: FreeRTOS architecture with Core 0 for UI, Core 1 for control
- **🔧 Smart Control**: PID-style temperature control with defrost cycles
- **🚨 Safety Systems**: Comprehensive fault detection and alarm management
- **📊 Service Interface**: Hidden technician menu with advanced diagnostics
- **💾 Memory Optimized**: Uses 8MB PSRAM for smooth graphics rendering

## 🏗️ Project Structure

```
Temperature_control/
├── 📋 platformio.ini              # Build configuration
├── 📁 include/                    # Header files
│   ├── config.h                   # Hardware & system config
│   ├── hmi_manager.h             # HMI interface
│   ├── io_controller.h           # I/O control interface  
│   └── lv_conf.h                 # LVGL configuration
├── 📁 src/                       # Source code
│   ├── main.cpp                  # Main application
│   ├── hmi_manager.cpp           # User interface
│   └── io_controller.cpp         # Relay & I/O control
├── 📁 lib/                       # Custom libraries
│   ├── ESP32_Display_Panel/      # ST7262 LCD driver
│   └── ESP32_IO_Expander/        # CH422G I/O expander
├── 📁 docs/                      # Technical documentation
│   ├── README.md                 # Original 89KB tech analysis
│   └── *.PDF                     # Hardware & design specs
└── 📄 BUILD_INSTRUCTIONS.md      # Detailed build guide
```

## 🔧 Quick Start - Compilation Instructions

### Method 1: PlatformIO (Recommended)

```bash
# 1. Install PlatformIO
pip install platformio

# 2. Navigate to project directory  
cd Temperature_control

# 3. Install dependencies and build
pio lib install
pio run

# 4. Upload to ESP32-S3 board
pio run --target upload

# 5. Monitor serial output
pio device monitor
```

### Method 2: Arduino IDE

1. **Install Arduino IDE** and ESP32 board support
2. **Install Libraries** (via Library Manager):
   - LVGL 8.3.11
   - PCF8574 by Rob Tillaart  
   - OneWire by Paul Stoffregen
   - DallasTemperature by Miles Burton
3. **Board Settings**:
   - Board: "ESP32S3 Dev Module"
   - Flash Size: 16MB
   - PSRAM: "OPI PSRAM"
4. Open `src/main.cpp` and compile

### Method 3: Quick Test

```bash
# Run the compilation helper script
chmod +x compile.sh
./compile.sh
```

## 🔌 Hardware Requirements

### Core Hardware
- **Waveshare ESP32-S3-Touch-LCD-4.3B** development board
- **PCF8574** I2C I/O expander for relay control
- **DS18B20** temperature sensors (1-4 sensors)
- Relay modules for compressor/fan/defrost control
- Buzzer for audible alarms

### Pin Configuration
```cpp
// I2C Bus (fixed on board)
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// Temperature Sensors
#define DS18B20_PIN 33       // 1-Wire bus pin

// Buzzer  
#define BUZZER_PIN 34        // Audible alarm

// I2C Device Addresses
#define IOEXP_I2C_ADDR 0x71  // CH422G (onboard)
#define PCF8574_I2C_ADDR 0x20 // PCF8574 (external)
```

## ⚙️ System Architecture

### Dual-Core FreeRTOS Design

**Core 0 (Protocol Core)** - High Priority UI Tasks:
- `lvgl_task`: Graphics rendering and touch input (Priority: 5)

**Core 1 (Application Core)** - Control & Sensors:  
- `control_logic_task`: Main control algorithm (Priority: 3)
- `sensor_task`: Temperature monitoring (Priority: 2)

### Memory Management
- **PSRAM**: LVGL graphics buffers and UI objects
- **Internal RAM**: FreeRTOS kernel and critical operations
- **Flash**: Application code and static assets

## 📱 User Interface Features

### Main Display
- **Large Temperature Display**: Easy-to-read actual temperature
- **Setpoint Control**: Touch buttons for temperature adjustment  
- **Status Indicators**: Visual feedback for system state
- **Alarm Notifications**: High-visibility warnings with silence option

### Service Menu (5-second hold top-right corner)
- **Live Sensor Data**: All 4 temperature readings
- **Control Parameters**: Setpoints, hysteresis, timing
- **Defrost Settings**: Interval, duration, termination temp
- **Alarm Configuration**: Thresholds and delays
- **System Diagnostics**: Fault codes and relay status

## 🛡️ Safety & Fault Detection

### Automatic Fault Detection
- **Sensor Failures**: Open/short circuit detection
- **Communication Errors**: I2C device monitoring
- **Relay Feedback**: Verification of relay operation
- **Temperature Alarms**: High/low temperature warnings

### Safety Actions
- **Fail-Safe Shutdown**: Automatic shutdown on critical faults
- **Alarm Escalation**: Visual → Audible → Service alert
- **Backup Modes**: Continued operation during sensor failures
- **Service Notifications**: Clear fault descriptions for technicians

## 🔧 Configuration & Customization

### Temperature Control Settings
```cpp
#define DEFAULT_SETPOINT_C -18.0f              // Target temperature
#define TEMP_HYSTERESIS_C 2.0f                 // Control deadband  
#define HIGH_TEMP_ALARM_DIFFERENTIAL_C 5.0f    // Alarm threshold
```

### Defrost Control
```cpp
#define DEFROST_INTERVAL_HOURS 6               // Auto defrost frequency
#define DEFROST_DURATION_MIN 20                // Defrost cycle time
#define DEFROST_TERMINATION_TEMP_C 10.0f       // End temperature
```

### UI Behavior
```cpp
#define MANUAL_DEFROST_HOLD_MS 3000            // Long-press duration
#define SERVICE_MENU_HOLD_MS 5000              // Service access time
#define ALARM_SILENCE_DURATION_MIN 20          // Silence timeout
```

## 🐛 Troubleshooting

### Common Build Issues

**LVGL Compilation Errors:**
```bash
# Ensure LVGL version is exactly 8.3.11
pio lib uninstall lvgl
pio lib install lvgl@8.3.11
```

**Memory Allocation Errors:**
- Verify PSRAM is enabled in `platformio.ini`
- Check `board_build.psram_mode = octal` setting
- Ensure proper MALLOC_CAP_SPIRAM usage

### Runtime Issues

**Black Screen:**
1. Check I2C communication (GPIO 8/9)
2. Verify CH422G at address 0x71
3. Confirm display reset sequence
4. Check backlight enable signal

**No Temperature Readings:**
1. Verify DS18B20 wiring on GPIO 33
2. Check 4.7kΩ pull-up resistor
3. Monitor serial for sensor detection

**Relay Control Problems:**
1. Confirm PCF8574 I2C address (0x20)
2. Check for I2C address conflicts
3. Verify relay wiring and power

## 📊 Serial Monitor Output

The system provides detailed diagnostic information:

```
===========================================
Commercial Freezer Controller Booting...
ESP32-S3-Touch-LCD-4.3B Platform
===========================================
1. Initializing I2C Bus...
   ✓ I2C Bus initialized successfully
2. Initializing CH422G I/O Expander...  
   ✓ CH422G initialized successfully
3. Performing Display Hardware Reset...
   ✓ Display reset sequence completed
4. Initializing Display Panel (ST7262)...
   ✓ Display panel initialized successfully
5. Enabling Display Backlight...
   ✓ Backlight enabled
...
===========================================
SYSTEM READY - Commercial Freezer Controller
===========================================
```

## 📚 Documentation

This project is based on comprehensive technical analysis:

- **README.md**: 89KB detailed hardware analysis and firmware architecture
- **Hardware interface.pdf**: Detailed hardware specifications
- **HMI Design.PDF**: User interface requirements  
- **Logic References.pdf**: Control algorithms and parameters
- **BUILD_INSTRUCTIONS.md**: Step-by-step compilation guide

## 🏭 Production Considerations

### Code Quality
- **Modular Architecture**: Separate files for different subsystems
- **Error Handling**: Comprehensive fault detection and recovery
- **Resource Management**: Proper mutex usage for thread safety
- **Memory Safety**: PSRAM allocation with fallback to internal RAM

### Performance Optimization
- **Dual-Core Utilization**: UI and control on separate cores
- **Efficient Graphics**: LVGL with hardware acceleration
- **Interrupt-Driven**: Touch and sensor processing
- **Watchdog Protection**: FreeRTOS task monitoring

## 🔄 Future Enhancements

### Connectivity Options
- **Wi-Fi/MQTT**: Remote monitoring and control
- **OTA Updates**: Over-the-air firmware updates
- **Data Logging**: SD card or cloud storage
- **Modbus/RS485**: Industrial communication protocols

### Advanced Features  
- **Energy Monitoring**: Current sensors for efficiency tracking
- **Predictive Maintenance**: Algorithm-based fault prediction
- **Multi-Zone Control**: Support for multiple freezer units
- **Advanced Analytics**: Performance trending and optimization

## 📄 License

This project implements a commercial-grade freezer controller based on detailed technical specifications. It demonstrates professional embedded systems development with modern UI, robust control algorithms, and comprehensive safety systems.

## 🤝 Contributing

This is a complete implementation based on comprehensive requirements analysis. The codebase is structured for easy modification and extension of functionality.

---

**Built with ❤️ for industrial IoT applications**  
*ESP32-S3 | LVGL | FreeRTOS | Modern Embedded Systems*