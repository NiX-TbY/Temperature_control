#ifndef CONFIG_H
#define CONFIG_H

// Display Configuration
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define SCREEN_ROTATION 1

// Pin Definitions for Waveshare ESP32-S3 4.3" Type B
#define TFT_BL 2
#define TFT_RST 4
#define TOUCH_INT 7
#define TOUCH_RST 5
#define TOUCH_SDA 6
#define TOUCH_SCL 7

// Sensor Pins
#define DHT_PIN 15
#define TEMP_SENSOR_PIN 16
#define RELAY_HEAT_PIN 17
#define RELAY_COOL_PIN 18
#define FAN_PWM_PIN 19
#define BUZZER_PIN 20

// System Configuration
#define DHT_TYPE DHT22
#define TEMP_UPDATE_INTERVAL 1000
#define DISPLAY_UPDATE_INTERVAL 500
#define SENSOR_READ_INTERVAL 2000

// Temperature Limits
#define MIN_TEMP 5.0
#define MAX_TEMP 40.0
#define DEFAULT_TARGET_TEMP 22.0
#define TEMP_TOLERANCE 0.5

// WiFi Configuration
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"
#define HOSTNAME "temperature-controller"

// Web Server
#define WEB_SERVER_PORT 80
#define WEBSOCKET_PORT 81

// Debug Configuration
#ifdef DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(x, ...) Serial.printf(x, __VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(x, ...)
#endif

#endif
