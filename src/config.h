#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>

// =================================================================
// == HARDWARE CONFIGURATION
// =================================================================

// I2C Bus Configuration (hardwired on Type B board)
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define I2C_FREQUENCY 100000

// CH422G I/O Expander (onboard for display control)
#define CH422G_I2C_ADDR 0x71
#define CH422G_RST_PIN 0    // Display Reset via expander
#define CH422G_BL_PIN 1     // Backlight Enable via expander

// PCF8574 I/O Expander (external for relay control)
#define PCF8574_I2C_ADDR 0x20
#define RELAY_COMPRESSOR_PIN    0
#define RELAY_EVAP_FAN_PIN      1
#define RELAY_DEFROST_HOTGAS_PIN 2
#define RELAY_DEFROST_ELEC_PIN  3
#define FEEDBACK_COMPRESSOR_PIN 4
#define FEEDBACK_EVAP_FAN_PIN   5

// GT911 Touch Controller
#define GT911_I2C_ADDR 0x5D
#define GT911_IRQ_PIN 4

// PCF85063A Real-Time Clock
#define RTC_I2C_ADDR 0x51

// Temperature Sensors (1-Wire DS18B20)
#define DS18B20_PIN 33  // Changed from GPIO15 to avoid CAN conflict

// Application GPIO pins (available on Type B board)
#define BUZZER_PIN 34
#define LED_STATUS_PIN 35
#define SPARE_GPIO1 36
#define SPARE_GPIO2 37

// Display Configuration
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480
#define DISPLAY_COLOR_DEPTH 16

// =================================================================
// == APPLICATION PARAMETERS
// =================================================================

// Temperature Control
#define DEFAULT_SETPOINT_C -18.0f
#define TEMP_HYSTERESIS_C 2.0f
#define TEMP_ALARM_DIFFERENTIAL_C 5.0f

// Timing Parameters
#define SENSOR_READ_INTERVAL_MS 2000
#define CONTROL_LOOP_INTERVAL_MS 1000
#define HMI_UPDATE_INTERVAL_MS 250
#define LVGL_TICK_INTERVAL_MS 5

// Alarm Configuration
#define ALARM_SILENCE_DURATION_MS (20 * 60 * 1000)  // 20 minutes
#define HIGH_TEMP_ALARM_DELAY_MS (5 * 60 * 1000)   // 5 minutes

// Defrost Configuration
#define DEFROST_INTERVAL_HOURS 6
#define DEFROST_DURATION_MS (20 * 60 * 1000)       // 20 minutes
#define DEFROST_TERMINATION_TEMP_C 10.0f
#define MANUAL_DEFROST_HOLD_TIME_MS 3000

// UI Configuration
#define SERVICE_MENU_HOLD_TIME_MS 5000

// FreeRTOS Task Priorities (higher number = higher priority)
#define TASK_PRIORITY_LVGL 5
#define TASK_PRIORITY_TOUCH 4
#define TASK_PRIORITY_CONTROL 3
#define TASK_PRIORITY_SENSORS 2
#define TASK_PRIORITY_LOGGING 1

// Memory Configuration
#define LVGL_BUFFER_SIZE (DISPLAY_WIDTH * 60)  // 60 lines buffer
#define TASK_STACK_SIZE_LVGL 8192
#define TASK_STACK_SIZE_CONTROL 4096
#define TASK_STACK_SIZE_SENSORS 3072
#define TASK_STACK_SIZE_LOGGING 4096

// =================================================================
// == SYSTEM STATE STRUCTURES
// =================================================================

enum class SystemState : uint8_t {
    STARTUP,
    NORMAL_OPERATION,
    DEFROST_CYCLE,
    ALARM_ACTIVE,
    FAULT_CONDITION,
    SERVICE_MODE
};

enum class AlarmState : uint8_t {
    NONE,
    HIGH_TEMP_ACTIVE,
    HIGH_TEMP_SILENCED,
    LOW_TEMP_ACTIVE,
    LOW_TEMP_SILENCED
};

enum class FaultCode : uint8_t {
    NONE = 0,
    // Sensor faults (priority 10-19)
    CABIN_SENSOR_OPEN = 10,
    CABIN_SENSOR_SHORT = 11,
    EVAP_SENSOR_FAIL = 12,
    CONDENSER_SENSOR_FAIL = 13,
    SUCTION_SENSOR_FAIL = 14,
    // Hardware faults (priority 20-29) 
    COMPRESSOR_FEEDBACK_FAIL = 20,
    EVAP_FAN_FEEDBACK_FAIL = 21,
    // Communication faults (priority 90-99)
    PCF8574_COMM_FAIL = 90,
    CH422G_COMM_FAIL = 91,
    RTC_COMM_FAIL = 92,
    TOUCH_COMM_FAIL = 93
};

enum class DefrostType : uint8_t {
    NONE,
    HOT_GAS,
    ELECTRIC,
    MANUAL
};

struct SensorData {
    float cabin_temp_c;
    float evap_temp_c;
    float condenser_temp_c;
    float suction_temp_c;
    bool sensor_valid[4];  // validity flags for each sensor
    uint32_t last_read_ms;
};

struct ControlSettings {
    float setpoint_temp_c;
    float hysteresis_c;
    float alarm_differential_c;
    uint16_t defrost_interval_hours;
    uint16_t defrost_duration_minutes;
    float defrost_termination_temp_c;
    DefrostType defrost_type;
    bool enable_adaptive_defrost;
};

struct SystemStatus {
    SystemState current_state;
    AlarmState alarm_state;
    FaultCode active_fault;
    bool compressor_running;
    bool evap_fan_running;
    bool defrost_active;
    bool manual_defrost_requested;
    uint32_t defrost_start_time;
    uint32_t last_defrost_time;
    uint32_t compressor_runtime_hours;
    uint16_t defrost_count_today;
};

// Global state structure protected by mutex
struct GlobalState {
    SemaphoreHandle_t mutex;
    SensorData sensors;
    ControlSettings settings;
    SystemStatus status;
    TimerHandle_t alarm_silence_timer;
    TimerHandle_t defrost_timer;
    bool service_menu_active;
    bool hmi_needs_update;
};

// Global state instance
extern GlobalState g_state;

// =================================================================
// == UTILITY MACROS
// =================================================================

#define SAFE_STATE_READ(member, value) \
    do { \
        if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(100)) == pdTRUE) { \
            value = g_state.member; \
            xSemaphoreGive(g_state.mutex); \
        } \
    } while(0)

#define SAFE_STATE_WRITE(member, value) \
    do { \
        if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(100)) == pdTRUE) { \
            g_state.member = value; \
            g_state.hmi_needs_update = true; \
            xSemaphoreGive(g_state.mutex); \
        } \
    } while(0)

// Debugging
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)

// Error checking
#define CHECK_ESP_ERROR(x) do { \
    esp_err_t err = (x); \
    if (err != ESP_OK) { \
        DEBUG_PRINTF("ESP Error: %s at %s:%d\n", esp_err_to_name(err), __FILE__, __LINE__); \
    } \
} while(0)