#!/bin/bash

# Temperature Control Project - Compilation Script
# This script demonstrates how to compile the project

echo "====================================================="
echo "Temperature Control Project - Build System"
echo "ESP32-S3-Touch-LCD-4.3B Commercial Freezer Controller"
echo "====================================================="

# Check if we're in the right directory
if [ ! -f "platformio.ini" ]; then
    echo "ERROR: platformio.ini not found. Please run this script from the project root directory."
    exit 1
fi

echo "✓ Project structure verified"

# Check project structure
echo ""
echo "Project Structure:"
echo "├── platformio.ini       (PlatformIO configuration)"
echo "├── include/            (Header files)"
echo "│   ├── config.h        (Hardware and system configuration)"
echo "│   ├── hmi_manager.h   (HMI interface definitions)"
echo "│   ├── io_controller.h (I/O controller interface)"
echo "│   └── lv_conf.h       (LVGL configuration)"
echo "├── src/                (Source code)"
echo "│   ├── main.cpp        (Main application)"
echo "│   ├── hmi_manager.cpp (Human-Machine Interface)"
echo "│   └── io_controller.cpp (Relay and I/O control)"
echo "├── lib/                (Custom libraries)"
echo "│   ├── ESP32_Display_Panel/ (Display driver library)"
echo "│   └── ESP32_IO_Expander/   (I/O expander library)"
echo "└── BUILD_INSTRUCTIONS.md   (Detailed build guide)"

echo ""
echo "Key Features Implemented:"
echo "✓ Dual-core FreeRTOS architecture"
echo "✓ LVGL 8.3.11 graphics library integration"
echo "✓ Touch-screen HMI with temperature display"
echo "✓ Multi-sensor temperature monitoring (DS18B20)"
echo "✓ Relay control via PCF8574 I2C expander"
echo "✓ Comprehensive fault detection and alarms"
echo "✓ Manual and automatic defrost cycles"
echo "✓ Service menu with advanced configuration"
echo "✓ High-temperature alarms with silence functionality"

echo ""
echo "Hardware Configuration:"
echo "• Target: ESP32-S3-Touch-LCD-4.3B (Waveshare)"
echo "• Display: 800x480 IPS LCD with ST7262 controller"
echo "• Touch: GT911 capacitive touch controller"
echo "• I/O Expansion: CH422G (onboard) + PCF8574 (external)"
echo "• Sensors: Up to 4x DS18B20 temperature sensors"
echo "• Memory: 16MB Flash + 8MB Octal PSRAM"

echo ""
echo "Build Commands:"
echo ""
echo "# Install PlatformIO (if not already installed)"
echo "pip install platformio"
echo ""
echo "# Install dependencies and build"
echo "pio lib install"
echo "pio run"
echo ""
echo "# Upload to board"
echo "pio run --target upload"
echo ""
echo "# Monitor serial output"
echo "pio device monitor"

echo ""
echo "Configuration Files:"
echo "• platformio.ini: Build configuration with ESP32-S3 settings"
echo "• include/config.h: Hardware pins and system parameters"
echo "• include/lv_conf.h: LVGL graphics library configuration"

echo ""
echo "Important Notes:"
echo "⚠️  This project requires specific hardware initialization sequence"
echo "⚠️  CH422G I/O expander must be initialized before display"
echo "⚠️  PSRAM must be enabled for LVGL graphics buffers"
echo "⚠️  Custom libraries in lib/ directory are mandatory"

echo ""
echo "For detailed instructions, see BUILD_INSTRUCTIONS.md"
echo "====================================================="

# Try to check if platformio is available
if command -v pio &> /dev/null; then
    echo ""
    echo "PlatformIO detected. You can now run:"
    echo "  pio run              # To build"
    echo "  pio run -t upload    # To build and upload"
else
    echo ""
    echo "PlatformIO not found. Install with: pip install platformio"
fi

echo "====================================================="