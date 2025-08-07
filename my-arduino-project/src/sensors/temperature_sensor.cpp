#include "sensors/temperature_sensor.h"

TemperatureSensor::TemperatureSensor() {
    _dht = new DHT(DHT_PIN, DHT_TYPE);
}

TemperatureSensor::~TemperatureSensor() {
    delete _dht;
}

bool TemperatureSensor::init() {
    DEBUG_PRINTLN("Initializing temperature sensor...");
    
    _dht->begin();
    delay(2000); // DHT needs time to stabilize
    
    // Test read
    if (readSensor()) {
        _initialized = true;
        DEBUG_PRINTLN("Temperature sensor initialized successfully");
        return true;
    } else {
        DEBUG_PRINTLN("Failed to initialize temperature sensor");
        return false;
    }
}

bool TemperatureSensor::readSensor() {
    if (!_initialized || millis() - _lastReadTime < SENSOR_READ_INTERVAL) {
        return _lastReading.isValid;
    }
    
    float temp = _dht->readTemperature();
    float humidity = _dht->readHumidity();
    
    if (isnan(temp) || isnan(humidity)) {
        _errorCount++;
        DEBUG_PRINTF("Sensor read error count: %d\n", _errorCount);
        
        if (_errorCount > 5) {
            _lastReading.isValid = false;
        }
        return false;
    }
    
    _errorCount = 0;
    _lastReading.temperature = temp;
    _lastReading.humidity = humidity;
    _lastReading.heatIndex = _dht->computeHeatIndex(temp, humidity, false);
    _lastReading.isValid = true;
    _lastReading.timestamp = millis();
    _lastReadTime = millis();
    
    DEBUG_PRINTF("Sensor reading - Temp: %.1f°C, Humidity: %.1f%%, Heat Index: %.1f°C\n", 
                 temp, humidity, _lastReading.heatIndex);
    
    return true;
}

void TemperatureSensor::calibrateTemperature(float offset) {
    if (_lastReading.isValid) {
        _lastReading.temperature += offset;
    }
}

bool TemperatureSensor::selfTest() {
    DEBUG_PRINTLN("Running sensor self-test...");
    
    for (int i = 0; i < 3; i++) {
        if (readSensor() && _lastReading.isValid) {
            if (_lastReading.temperature > -40 && _lastReading.temperature < 80 &&
                _lastReading.humidity >= 0 && _lastReading.humidity <= 100) {
                DEBUG_PRINTLN("Sensor self-test passed");
                return true;
            }
        }
        delay(1000);
    }
    
    DEBUG_PRINTLN("Sensor self-test failed");
    return false;
}

TemperatureSensor tempSensor;
