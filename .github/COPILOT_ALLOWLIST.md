# GitHub Copilot Coding Agent Allowlist Configuration
# This file documents the required URLs and hosts for the Temperature Control project
# Repository admins should add these to the Copilot coding agent settings

## Required External Resources for ESP32-S3 Temperature Control Project

### PlatformIO and ESP32 Platform
- registry.platformio.org
- dl.espressif.com
- github.com/espressif/*
- api.registry.platformio.org

### Arduino ESP32 Framework
- github.com/espressif/arduino-esp32
- raw.githubusercontent.com/espressif/*

### Essential Libraries
- github.com/lovyan03/LovyanGFX
- github.com/bblanchon/ArduinoJson
- github.com/adafruit/Adafruit_Sensor
- github.com/adafruit/DHT-sensor-library
- github.com/me-no-dev/ESPAsyncWebServer
- github.com/me-no-dev/AsyncTCP

### Package Managers and Tools
- pypi.org
- files.pythonhosted.org
- esptool.readthedocs.io

### Documentation and Resources
- docs.platformio.org
- docs.espressif.com
- arduino-esp32.readthedocs.io
- lovyangfx.readthedocs.io

### Build Tools and Compilers
- github.com/espressif/crosstool-NG
- dl.espressif.com/dl/
- github.com/espressif/esp-idf

## Instructions for Repository Admins

1. Navigate to your repository settings
2. Go to "Copilot" section
3. Find "Coding agent settings" 
4. Add the following URLs to the custom allowlist:

### Core Development URLs (High Priority)
```
registry.platformio.org
dl.espressif.com
github.com/espressif/arduino-esp32
github.com/lovyan03/LovyanGFX
github.com/bblanchon/ArduinoJson
github.com/adafruit/Adafruit_Sensor
github.com/adafruit/DHT-sensor-library
api.registry.platformio.org
pypi.org
files.pythonhosted.org
```

### Documentation URLs (Medium Priority)
```
docs.platformio.org
docs.espressif.com
arduino-esp32.readthedocs.io
lovyangfx.readthedocs.io
esptool.readthedocs.io
```

### Extended Library Support (Lower Priority)
```
github.com/me-no-dev/ESPAsyncWebServer
github.com/me-no-dev/AsyncTCP
github.com/espressif/crosstool-NG
raw.githubusercontent.com/espressif/*
```

## Security Considerations

- All URLs are from trusted sources (official repositories, package managers)
- ESP32 and PlatformIO are industry-standard platforms
- Libraries are well-maintained and widely used in IoT projects
- Regular security updates are available through dependabot

## Project-Specific Needs

This Temperature Control project requires:
- ESP32-S3 platform support
- LovyanGFX for display management
- DHT sensor libraries for temperature monitoring
- Async web server for remote monitoring
- JSON handling for configuration and API

Last Updated: August 7, 2025
