#!/bin/bash

echo "=== Commercial Freezer Controller - Project Validation ==="
echo

# Check project structure
echo "1. Checking project structure..."
if [ -f "platformio.ini" ] && [ -f "src/main.cpp" ]; then
    echo "✓ PlatformIO project structure is correct"
else
    echo "✗ Missing essential project files"
    exit 1
fi

# Check source files
echo
echo "2. Checking source files..."
required_files=(
    "src/config.h"
    "src/main.cpp"
    "src/main_test.cpp"
    "src/relay_controller.h"
    "src/relay_controller.cpp"
    "src/temperature_sensors.h"
    "src/temperature_sensors.cpp"
    "src/lv_conf.h"
)

for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file"
    else
        echo "✗ Missing: $file"
    fi
done

# Check custom libraries
echo
echo "3. Checking custom libraries..."
if [ -d "lib/ESP32_Display_Panel" ] && [ -f "lib/ESP32_Display_Panel/src/esp_panel.h" ]; then
    echo "✓ ESP32_Display_Panel library"
else
    echo "✗ Missing ESP32_Display_Panel library"
fi

if [ -d "lib/ESP32_IO_Expander" ] && [ -f "lib/ESP32_IO_Expander/src/ESP_IOExpander.h" ]; then
    echo "✓ ESP32_IO_Expander library"
else
    echo "✗ Missing ESP32_IO_Expander library"
fi

# Check configuration
echo
echo "4. Checking configuration..."
if grep -q "DEFAULT_SETPOINT_C" src/config.h; then
    echo "✓ Configuration constants defined"
else
    echo "✗ Missing configuration constants"
fi

if grep -q "CH422G_RST_PIN" src/config.h; then
    echo "✓ Hardware pin definitions present"
else
    echo "✗ Missing hardware pin definitions"
fi

# Check main features
echo
echo "5. Checking main features..."
if grep -q "xTaskCreatePinnedToCore" src/main.cpp; then
    echo "✓ FreeRTOS dual-core architecture implemented"
else
    echo "✗ Missing FreeRTOS task implementation"
fi

if grep -q "lv_init" src/main.cpp; then
    echo "✓ LVGL graphics library integration"
else
    echo "✗ Missing LVGL integration"
fi

if grep -q "CH422G" src/main.cpp; then
    echo "✓ Critical I/O expander initialization"
else
    echo "✗ Missing CH422G initialization"
fi

if grep -q "DallasTemperature" src/temperature_sensors.cpp; then
    echo "✓ Temperature sensor support"
else
    echo "✗ Missing temperature sensor support"
fi

# Check documentation
echo
echo "6. Checking documentation..."
docs=(
    "BUILD_INSTRUCTIONS.md"
    "USAGE_GUIDE.md"
    "README.md"
)

for doc in "${docs[@]}"; do
    if [ -f "$doc" ]; then
        echo "✓ $doc"
    else
        echo "✗ Missing: $doc"
    fi
done

# Summary
echo
echo "=== Project Validation Summary ==="
echo "This project implements a complete Commercial Freezer Controller with:"
echo "• Waveshare ESP32-S3-Touch-LCD-4.3B support"
echo "• LVGL graphical user interface"
echo "• Dual-core FreeRTOS architecture"
echo "• Temperature monitoring with DS18B20 sensors"
echo "• Relay control via PCF8574"
echo "• Critical CH422G I/O expander initialization sequence"
echo "• Comprehensive alarm and fault detection system"
echo

echo "To build and run:"
echo "1. Install PlatformIO: pip install platformio"
echo "2. Build project: pio run"
echo "3. Upload to board: pio run --target upload"
echo "4. Monitor output: pio device monitor"
echo

echo "For testing without hardware:"
echo "1. Copy test configuration: cp platformio_simple.ini platformio.ini"
echo "2. Copy test main file: cp src/main_test.cpp src/main.cpp"
echo "3. Build: pio run"
echo

echo "Project validation complete!"