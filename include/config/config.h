#pragma once

#include <Arduino.h>

// Pin definitions
#define RELAY_HEAT_PIN 2
#define RELAY_COOL_PIN 3
#define FAN_PWM_PIN 4
#define BUZZER_PIN 5

// Temperature limits
#define MIN_TEMP -40.0
#define MAX_TEMP 80.0

// Control parameters
#define TEMP_HYSTERESIS_C 2.0

// Control timing
#define TEMP_UPDATE_INTERVAL 1000  // ms

// Debug macros
#ifdef DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(fmt, ...)
#endif