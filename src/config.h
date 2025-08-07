#pragma once

// =================================================================
// == HARDWARE & SYSTEM CONFIGURATION
// =================================================================

// --- I2C Bus Configuration ---
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// --- CH422G I/O Expander (Onboard, for Display/SD Control) ---
#define IOEXP_I2C_ADDR 0x71
#define CH422G_RST_PIN 0 // Display Reset
#define CH422G_BL_EN_PIN 1 // Backlight Enable

// --- PCF8574 I/O Expander (External, for Relay Control) ---
#define PCF8574_I2C_ADDR 0x20
#define RELAY_PIN_COMPRESSOR    0
#define RELAY_PIN_EVAP_FAN      1
#define RELAY_PIN_DEFROST_HOTGAS 2
#define RELAY_PIN_DEFROST_ELEC  3
#define FEEDBACK_PIN_COMPRESSOR 4
#define FEEDBACK_PIN_EVAP_FAN   5

// --- 1-Wire Temperature Sensors ---
#define DS18B20_PIN 33 // Corrected GPIO, avoiding conflict with CAN on GPIO15

// --- Buzzer Control ---
#define BUZZER_PIN 34 // Available GPIO from master pinout

// =================================================================
// == APPLICATION & CONTROL LOGIC PARAMETERS
// =================================================================

// --- Temperature Control ---
#define DEFAULT_SETPOINT_C -18.0f
#define TEMP_HYSTERESIS_C 2.0f

// --- Alarm Configuration ---
#define HIGH_TEMP_ALARM_DIFFERENTIAL_C 5.0f
#define ALARM_SILENCE_DURATION_MIN 20

// --- Defrost Configuration ---
#define DEFROST_INTERVAL_HOURS 6
#define DEFROST_DURATION_MIN 20
#define DEFROST_TERMINATION_TEMP_C 10.0f
#define MANUAL_DEFROST_HOLD_MS 3000

// --- UI Configuration ---
#define SERVICE_MENU_HOLD_MS 5000

// =================================================================
// == GLOBAL ENUMS & STRUCTURES
// =================================================================
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// System alarm states
enum class AlarmState {
    NONE,
    HIGH_TEMP_ACTIVE,
    HIGH_TEMP_SILENCED
};

// System fault codes (prioritized)
enum class FaultCode {
    NONE = 0,
    HIGH_TEMP_ALARM = 1,
    CONDENSER_SENSOR_FAIL = 10,
    EVAP_SENSOR_FAIL = 11,
    CABIN_SENSOR_SHORT = 20,
    CABIN_SENSOR_OPEN = 21,
    COMPRESSOR_FEEDBACK_MISMATCH = 30,
    IO_EXPANDER_FAIL = 99
};

// Global state structure for safe inter-task communication
struct SystemState {
    SemaphoreHandle_t mutex;
    float actual_temp_celsius;
    float setpoint_temp_celsius;
    float sensor_values[4]; // 0:Cabin, 1:Evap, 2:Condenser, 3:Suction
    AlarmState alarm_status;
    bool defrost_active;
    FaultCode active_fault_code;
    uint8_t relay_states; // Bitmask of active relays
};

extern SystemState g_system_state;