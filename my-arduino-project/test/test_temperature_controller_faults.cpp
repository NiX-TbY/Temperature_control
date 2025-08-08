#include <unity.h>
#include "controllers/temperature_controller.h"
#include "types/types.h"
#include "config/config.h"

// Helper to fabricate sensors array
static SensorData makeSensor(float t, bool valid=true) {
    SensorData s{}; s.temperature = t; s.valid = valid; s.humidity = 0; s.timestamp = millis(); s.lastValidReading = s.timestamp; s.sensorId = 0; s.address = ""; return s;
}

void setUp() {
    // Reconstruct controller to clear internal timers/state (simple approach)
    controller = TemperatureController();
    controller.init();
    controller.setTargetTemperature(-10.0f);
}

void tearDown() {}

void advance(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        // yield loop
    }
}

void test_over_temperature_fault() {
    SensorData sensors[1] = { makeSensor(controller.getConfig().targetTemp + OVER_TEMPERATURE_MARGIN + 1.0f) };
    // Repeated updates until debounce window passed
    unsigned long start = millis();
    while (millis() - start < FAULT_DEBOUNCE_MS + 10) {
        controller.updateWithMultipleSensors(sensors, 1);
    }
    TEST_ASSERT_TRUE(controller.faultActive(FaultBit(FAULT_OVER_TEMPERATURE_BIT)));
}

void test_under_temperature_fault() {
    SensorData sensors[1] = { makeSensor(controller.getConfig().targetTemp - UNDER_TEMPERATURE_MARGIN - 1.0f) };
    unsigned long start = millis();
    while (millis() - start < FAULT_DEBOUNCE_MS + 10) {
        controller.updateWithMultipleSensors(sensors, 1);
    }
    TEST_ASSERT_TRUE(controller.faultActive(FaultBit(FAULT_UNDER_TEMPERATURE_BIT)));
}

void test_sensor_missing_fault() {
    SensorData sensors[1] = { makeSensor(0,false) }; // invalid sensor
    unsigned long start = millis();
    while (millis() - start < SENSOR_MISSING_DEBOUNCE_MS + 10) {
        controller.updateWithMultipleSensors(sensors, 1);
    }
    TEST_ASSERT_TRUE(controller.faultActive(FaultBit(FAULT_SENSOR_MISSING_BIT)));
}

void test_range_fault() {
    SensorData sensors[1] = { makeSensor(TEMP_MAX_SAFE + 20.0f, true) };
    unsigned long start = millis();
    while (millis() - start < RANGE_FAULT_DEBOUNCE_MS + 10) {
        controller.updateWithMultipleSensors(sensors, 1);
    }
    TEST_ASSERT_TRUE(controller.faultActive(FaultBit(FAULT_SENSOR_RANGE_BIT)));
}

void test_alarm_escalation_from_overtemp() {
    SensorData sensors[1] = { makeSensor(controller.getConfig().targetTemp + OVER_TEMPERATURE_MARGIN + 2.0f) };
    unsigned long start = millis();
    while (millis() - start < (FAULT_DEBOUNCE_MS + ALARM_TRIGGER_GRACE_MS + 20)) {
        controller.updateWithMultipleSensors(sensors, 1);
    }
    TEST_ASSERT_TRUE(controller.getState().alarmActive);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_over_temperature_fault);
    RUN_TEST(test_under_temperature_fault);
    RUN_TEST(test_sensor_missing_fault);
    RUN_TEST(test_range_fault);
    RUN_TEST(test_alarm_escalation_from_overtemp);
    return UNITY_END();
}
