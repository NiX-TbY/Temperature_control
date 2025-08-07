# Commercial Freezer Controller

A sophisticated temperature control system for commercial freezers built on the Waveshare ESP32-S3-Touch-LCD-4.3B development board.

## Hardware Requirements

### Main Board
- **Waveshare ESP32-S3-Touch-LCD-4.3B** development board
  - ESP32-S3-WROOM-1-N16R8 module (16MB Flash, 8MB PSRAM)
  - 4.3" 800×480 IPS display with ST7262 controller
  - GT911 capacitive touch controller
  - CH422G I/O expander (onboard)
  - PCF85063A RTC

### External Components
- **PCF8574 I/O Expander** (for relay control)
- **DS18B20 Temperature Sensors** (up to 4 sensors)
  - Cabin temperature sensor
  - Evaporator temperature sensor  
  - Condenser temperature sensor
  - Suction line temperature sensor
- **Relays** with auxiliary contacts for feedback
  - Compressor relay
  - Evaporator fan relay
  - Hot gas defrost relay
  - Electric defrost relay (optional)
- **Buzzer** for alarms

## Wiring Connections

### I2C Bus (Shared)
- **SDA:** GPIO 8
- **SCL:** GPIO 9
- **Devices:** GT911 (0x5D), CH422G (0x71), PCF85063A (0x51), PCF8574 (0x20)

### Temperature Sensors
- **1-Wire Bus:** GPIO 33 (DS18B20 sensors)

### Buzzer
- **Control Pin:** GPIO 34

### PCF8574 Pin Assignments
| Pin | Function | Type |
|-----|----------|------|
| P0  | Compressor Relay | Output |
| P1  | Evaporator Fan Relay | Output |
| P2  | Hot Gas Defrost Relay | Output |
| P3  | Electric Defrost Relay | Output |
| P4  | Compressor Feedback | Input |
| P5  | Fan Feedback | Input |
| P6-P7 | Reserved | - |

## Software Architecture

### Dual-Core Task Distribution
- **Core 0 (Protocol Core):** LVGL UI rendering and touch handling
- **Core 1 (Application Core):** Control logic, sensors, and I/O management

### Key Features
- **LVGL-based GUI** with 800×480 resolution
- **Dual-buffered graphics** using PSRAM for smooth rendering
- **Multi-sensor temperature monitoring** with fault detection
- **Hysteresis-based temperature control**
- **Manual and automatic defrost cycles**
- **Visual and audible alarms**
- **Relay feedback verification**
- **Service menu** for configuration

## Building the Project

### Prerequisites
1. **PlatformIO IDE** or **PlatformIO Core CLI**
2. **ESP32 platform** support in PlatformIO

### Installation Steps

1. **Clone the repository:**
   ```bash
   git clone https://github.com/NiX-TbY/Temperature_control.git
   cd Temperature_control
   ```

2. **Install PlatformIO (if not already installed):**
   ```bash
   # Using pip
   pip install platformio
   
   # Or using the installer script
   curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py -o get-platformio.py
   python get-platformio.py
   ```

3. **Build the project:**
   ```bash
   pio run
   ```

4. **Upload to the board:**
   ```bash
   pio run --target upload
   ```

5. **Monitor serial output:**
   ```bash
   pio device monitor
   ```

### Build Targets
- `pio run` - Compile the firmware
- `pio run -t clean` - Clean build files
- `pio run -t upload` - Upload to connected board
- `pio run -t monitor` - Monitor serial output

## Hardware Setup

### Critical Initialization Sequence
The system requires a specific initialization order due to hardware dependencies:

1. **I²C Bus** initialization
2. **CH422G I/O Expander** setup
3. **Display hardware reset** via CH422G
4. **ST7262 display controller** initialization
5. **Backlight enable** via CH422G

> ⚠️ **Important:** The display reset and backlight control are managed by the onboard CH422G I/O expander, not direct GPIO pins.

### Library Dependencies
The project uses specific library versions for compatibility:
- **ESP32_Display_Panel@1.0.0** (local)
- **ESP32_IO_Expander@1.0.1** (local)
- **LVGL@8.3.11**
- **DallasTemperature@3.11.0**
- **OneWire@2.3.8**
- **PCF8574@0.4.2**

## Usage Instructions

### Basic Operation
1. **Power on** the system
2. **Monitor temperature** on the large display
3. **Adjust setpoint** using up/down buttons
4. **Manual defrost** by holding defrost button for 3 seconds

### Service Menu Access
- **Long press** (5 seconds) the top-right corner of the screen
- Access advanced settings and diagnostics

### Alarm Handling
- **High temperature alarm** triggers visual and audible alerts
- **Silence alarm** by pressing the silence button (20-minute silence period)
- **Fault codes** displayed in place of setpoint when critical faults occur

### Temperature Control Logic
- **Hysteresis control:** ±2°C around setpoint
- **High temperature alarm:** +5°C above setpoint
- **Automatic defrost:** Every 6 hours or based on evaporator temperature

## Troubleshooting

### Black Screen on Boot
1. Check I²C connections (GPIO 8, 9)
2. Verify CH422G I/O expander communication
3. Ensure proper power supply (5V/2A minimum)

### Temperature Sensor Issues
- Check 1-Wire connections on GPIO 33
- Verify sensor addresses in serial monitor
- Replace faulty sensors showing -127°C or 85°C

### Relay Control Problems
- Verify PCF8574 I²C address (default 0x20)
- Check relay wiring and auxiliary contacts
- Monitor feedback signals for proper operation

### Build Issues
- Ensure PlatformIO is up to date
- Clean and rebuild: `pio run -t clean && pio run`
- Check library dependencies

## Configuration

### Key Parameters (config.h)
```cpp
#define DEFAULT_SETPOINT_C -18.0f        // Default temperature setpoint
#define TEMP_HYSTERESIS_C 2.0f           // Control hysteresis
#define HIGH_TEMP_ALARM_DIFFERENTIAL_C 5.0f  // Alarm threshold
#define DEFROST_INTERVAL_HOURS 6         // Defrost frequency
#define MANUAL_DEFROST_HOLD_MS 3000      // Button hold time
```

### I2C Device Addresses
- **CH422G (onboard):** 0x71
- **PCF8574 (external):** 0x20
- **GT911 Touch:** 0x5D
- **PCF85063A RTC:** 0x51

## Development Notes

### Memory Usage
- **PSRAM allocation:** Graphics buffers and LVGL heap
- **Internal RAM:** FreeRTOS kernel and critical variables
- **Flash:** Application code and static assets

### Real-time Constraints
- **LVGL task:** 5ms priority on Core 0
- **Control task:** 1000ms cycle on Core 1
- **Sensor task:** 2000ms cycle on Core 1

### Safety Features
- **Watchdog protection** for critical tasks
- **Relay feedback verification** prevents unsafe operation
- **Fault prioritization** ensures critical issues are visible
- **Graceful degradation** in sensor failure scenarios

## License

This project is provided as-is for educational and commercial use. Please respect the licenses of the included libraries and components.

## Support

For technical support or questions about this implementation, please refer to the comprehensive documentation in the `README.md` file or create an issue in the project repository.

---

**Note:** This is a production-grade implementation designed for commercial freezer applications. Proper testing and validation should be performed before deployment in critical applications.