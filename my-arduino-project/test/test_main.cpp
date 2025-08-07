#include <unity.h>
#include "sensors/temperature_sensor.h"
#include "controllers/temperature_controller.h"
#include "display/display_driver.h"

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

void test_temperature_sensor_init() {
    TEST_ASSERT_TRUE(tempSensor.init());
}

void test_temperature_controller_init() {
    TEST_ASSERT_TRUE(controller.init());
}

void test_display_init() {
    TEST_ASSERT_TRUE(display.init());
}

void test_temperature_range_validation() {
    controller.setTargetTemperature(25.0);
    TEST_ASSERT_EQUAL_FLOAT(25.0, controller.getTargetTemperature());
    
    controller.setTargetTemperature(-10.0); // Below minimum
    TEST_ASSERT_EQUAL_FLOAT(MIN_TEMP, controller.getTargetTemperature());
    
    controller.setTargetTemperature(50.0); // Above maximum
    TEST_ASSERT_EQUAL_FLOAT(MAX_TEMP, controller.getTargetTemperature());
}

void test_mode_switching() {
    controller.setMode(MODE_HEAT);
    TEST_ASSERT_EQUAL(MODE_HEAT, controller.getMode());
    
    controller.setMode(MODE_COOL);
    TEST_ASSERT_EQUAL(MODE_COOL, controller.getMode());
    
    controller.setMode(MODE_AUTO);
    TEST_ASSERT_EQUAL(MODE_AUTO, controller.getMode());
}

void test_sensor_data_validity() {
    SensorData data = tempSensor.getData();
    if (data.isValid) {
        TEST_ASSERT_TRUE(data.temperature > -50 && data.temperature < 100);
        TEST_ASSERT_TRUE(data.humidity >= 0 && data.humidity <= 100);
    }
}

void test_safety_limits() {
    // Test that controller stops when temperature is out of range
    SensorData fakeData;
    fakeData.temperature = -100; // Way out of range
    fakeData.isValid = true;
    
    controller.update(fakeData);
    TEST_ASSERT_EQUAL(STATUS_ERROR, controller.getState().status);
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_temperature_sensor_init);
    RUN_TEST(test_temperature_controller_init);
    RUN_TEST(test_display_init);
    RUN_TEST(test_temperature_range_validation);
    RUN_TEST(test_mode_switching);
    RUN_TEST(test_sensor_data_validity);
    RUN_TEST(test_safety_limits);
    
    return UNITY_END();
}
