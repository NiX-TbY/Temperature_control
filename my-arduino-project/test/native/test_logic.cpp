#include <unity.h>
#include <cmath>

// Arduino minimal stubs
unsigned long fakeMillis = 0;
extern "C" unsigned long millis() { return fakeMillis; }
extern "C" void pinMode(int, int) {}
extern "C" void digitalWrite(int, int) {}
extern "C" void ledcSetup(int,int,int) {}
extern "C" void ledcAttachPin(int,int) {}
extern "C" void ledcWrite(int,int) {}
extern "C" void delay(unsigned long ms) { fakeMillis += ms; }

// Include headers then implementation of controller only
#include "controllers/temperature_controller.h"
#include "config/config.h"
#include "types/types.h"
#include "../src/controllers/temperature_controller.cpp"

static SensorData makeSensor(float t, bool valid=true) { SensorData s{}; s.temperature=t; s.valid=valid; return s; }

void test_pid_symmetry() {
    TemperatureController c; c.init(); c.setTargetTemperature(-10.0f);
    SensorData s[1];
    // Over target -> cooling path
    s[0]=makeSensor(-2.0f,true);
    for (int i=0;i<50;i++){ fakeMillis+=TEMP_UPDATE_INTERVAL; c.updateWithMultipleSensors(s,1);}    
    TEST_ASSERT_TRUE(c.getState().coolingActive);
    // Under target -> heating path
    s[0]=makeSensor(-25.0f,true);
    for (int i=0;i<50;i++){ fakeMillis+=TEMP_UPDATE_INTERVAL; c.updateWithMultipleSensors(s,1);}    
    TEST_ASSERT_TRUE(c.getState().heatingActive);
}

void test_overtemp_fault_fast() {
    TemperatureController c; c.init(); c.setTargetTemperature(-10.0f);
    SensorData s[1]= { makeSensor(5.0f,true) }; // 15C above target
    while (fakeMillis < FAULT_DEBOUNCE_MS + 10) { fakeMillis+=TEMP_UPDATE_INTERVAL; c.updateWithMultipleSensors(s,1);}    
    TEST_ASSERT_TRUE(c.faultActive(FaultBit(FAULT_OVER_TEMPERATURE_BIT)));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_pid_symmetry);
    RUN_TEST(test_overtemp_fault_fast);
    return UNITY_END();
}
