#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Optional DHT sensor support (excluded during minimal display-only build)
#ifdef ENABLE_DHT
  #include <DHT.h>
#else
  // Provide minimal stand-ins so rest of code referencing DHT_TYPE compiles
  #ifndef DHT22
    #define DHT22 22
  #endif
#endif

// System Version
#define SYSTEM_VERSION "1.0.0"
#define BOARD_TYPE "Waveshare ESP32-S3-Touch-LCD-4.3B"

// Display Configuration (Waveshare Type B specific)
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480
#define DISPLAY_BL_PWM_CHANNEL 0
#define DISPLAY_BL_PWM_FREQ 5000
#define DISPLAY_BL_PWM_RESOLUTION 8

// I2C Configuration (Critical for Type B)
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define I2C_FREQ 400000

// I2C Device Addresses
#define CH422G_ADDRESS 0x71    // I/O Expander (controls display reset & backlight)
#define GT911_ADDRESS_1 0x5D    // Touch controller primary address
#define GT911_ADDRESS_2 0x14    // Touch controller secondary address
#define PCF85063_ADDRESS 0x51   // RTC

// Touch Configuration
#define TOUCH_IRQ_PIN 4
#define TOUCH_MAX_POINTS 5

// Temperature Sensor Configuration
// Note: GPIO 15 is conflicted with display B7, using GPIO 33 instead
#define DS18B20_PIN 33

// DHT Sensor Configuration
#define DHT_PIN 32              // DHT22 sensor pin
#define DHT_TYPE DHT22          // DHT22 (AM2302)
#define SENSOR_READ_INTERVAL 2000  // DHT read interval in ms

// Display Constants
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480
#define I2C_FREQ 400000

// Debug settings
#define DEBUG_MODE
#define MAX_SENSORS 4
#define TEMP_READ_INTERVAL 1000 // ms
#define SENSOR_TIMEOUT 5000     // ms

// Control Pins (using available GPIOs)
#define RELAY_HEAT_PIN 34       // USER_GPIO_2
#define RELAY_COOL_PIN 35       // USER_GPIO_3
#define FAN_PWM_PIN 36          // USER_GPIO_4
#define BUZZER_PIN 37           // USER_GPIO_5

// Temperature Control Parameters
#define DEFAULT_TARGET_TEMP -18.0f
#define TEMP_DEADBAND 1.0f
#define TEMP_MIN_SAFE -30.0f
#define TEMP_MAX_SAFE 10.0f
#define TEMP_TOLERANCE 0.5f
#define MIN_TEMP -30.0f  // Minimum allowed temperature
#define MAX_TEMP 10.0f   // Maximum allowed temperature
#define DEFAULT_FAN_SPEED 50
#define TEMP_UPDATE_INTERVAL 1000  // ms

// Fault / Alarm Thresholds & Debounce (tunable)
#define OVER_TEMPERATURE_MARGIN 5.0f          // Degrees above target before over-temp fault window starts
#define UNDER_TEMPERATURE_MARGIN 5.0f         // Degrees below target before under-temp fault window starts
#define FAULT_DEBOUNCE_MS 5000                // Generic debounce for persistent condition (5s)
#define SENSOR_MISSING_DEBOUNCE_MS 3000       // Time without any valid sensor before fault
#define RANGE_FAULT_DEBOUNCE_MS 1000          // Out-of-safe-range persistence
#define DEFROST_TIMEOUT_GRACE_MS 60000        // Additional grace after configured duration before timeout fault
#define MIN_COMPRESSOR_OFF_TIME_MS 180000     // 3 minutes minimum off time (short cycle protection)
#define MIN_COMPRESSOR_ON_TIME_MS 60000       // 1 minute minimum on time (avoid rapid toggling)

// Test overrides: when building under UNIT_TEST reduce long debounce / intervals to keep tests fast.
#ifdef UNIT_TEST
  #undef FAULT_DEBOUNCE_MS
  #define FAULT_DEBOUNCE_MS 30
  #undef SENSOR_MISSING_DEBOUNCE_MS
  #define SENSOR_MISSING_DEBOUNCE_MS 30
  #undef RANGE_FAULT_DEBOUNCE_MS
  #define RANGE_FAULT_DEBOUNCE_MS 30
  #undef DEFROST_TIMEOUT_GRACE_MS
  #define DEFROST_TIMEOUT_GRACE_MS 120
  #undef MIN_COMPRESSOR_OFF_TIME_MS
  #define MIN_COMPRESSOR_OFF_TIME_MS 120
  #undef MIN_COMPRESSOR_ON_TIME_MS
  #define MIN_COMPRESSOR_ON_TIME_MS 60
  #undef ALARM_TRIGGER_GRACE_MS
  #define ALARM_TRIGGER_GRACE_MS 120
  #undef ALARM_SILENCE_DURATION_MS
  #define ALARM_SILENCE_DURATION_MS 240
  #undef TEMP_UPDATE_INTERVAL
  #define TEMP_UPDATE_INTERVAL 50
#endif

// Alarm System Timing
#define ALARM_TRIGGER_GRACE_MS 60000          // Over/under temp fault must persist this long before escalating to alarm state
#define ALARM_SILENCE_DURATION_MS 900000      // 15 minutes silence period default
#define ALARM_PULSE_INTERVAL_MS 1000          // Pulse interval for temperature display flashing

// FreeRTOS Task Configuration
#define DISPLAY_TASK_STACK 8192
#define TOUCH_TASK_STACK 4096
#define LVGL_TASK_STACK 8192
#define CONTROL_TASK_STACK 4096
#define SENSOR_TASK_STACK 4096
#define LOGGING_TASK_STACK 4096

// Task Priorities (Core 0 - UI, Core 1 - Application)
#define DISPLAY_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define TOUCH_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define LVGL_TASK_PRIORITY (configMAX_PRIORITIES - 3)
#define CONTROL_TASK_PRIORITY (tskIDLE_PRIORITY + 3)
#define SENSOR_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define LOGGING_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

// Memory Allocation (PSRAM)
#define LVGL_MEM_SIZE (2 * 1024 * 1024)  // 2MB for LVGL
#define FRAME_BUFFER_SIZE (800 * 480 * 2) // 16-bit color

// WiFi Configuration
#define WIFI_SSID "YourWiFiNetwork"      // Default WiFi network name
#define WIFI_PASSWORD "YourPassword"     // Default WiFi password  
#define HOSTNAME "ESP32-TempControl"     // Device hostname

// Color Constants (TFT color definitions)
#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F
#define TFT_YELLOW      0xFFE0
#define TFT_ORANGE      0xFD20
#define TFT_DARKRED     0x7800
#define TFT_DARKBLUE    0x000F
#define TFT_DARKGREEN   0x03E0

// Debug Configuration
#ifdef DEBUG_MODE
  #define DEBUG_SERIAL Serial
  #define DEBUG_PRINT(x) DEBUG_SERIAL.print(x)
  #define DEBUG_PRINTLN(x) DEBUG_SERIAL.println(x)
  #define DEBUG_PRINTF(x, ...) DEBUG_SERIAL.printf(x, ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, ...)
#endif

#endif // CONFIG_H