#include <unity.h>

// Forward declarations of test functions defined in other compilation units
void test_logging_constants_present();
void test_low_memory_guard();
void test_over_temperature_fault();
void test_under_temperature_fault();
void test_sensor_missing_fault();
void test_range_fault();
void test_alarm_escalation_from_overtemp();

// Arduino framework expects setup/loop symbols even in test context
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_logging_constants_present);
    RUN_TEST(test_low_memory_guard);
    RUN_TEST(test_over_temperature_fault);
    RUN_TEST(test_under_temperature_fault);
    RUN_TEST(test_sensor_missing_fault);
    RUN_TEST(test_range_fault);
    RUN_TEST(test_alarm_escalation_from_overtemp);
    UNITY_END();
}

void loop() {
    // Not used in unit tests
}
