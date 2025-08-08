#include "sensors/temperature_sensor.h"

// Global sensor instance
TemperatureSensor tempSensor;

TemperatureSensor::TemperatureSensor() {
    _oneWire = nullptr;
    _dallas = nullptr;
    _sensorCount = 0;
    _lastReadTime = 0;
    
    // Initialize sensor data structures
    for (int i = 0; i < MAX_SENSORS; i++) {
        _sensors[i].temperature = 0.0;
        _sensors[i].humidity = 0.0;
        _sensors[i].valid = false;
        _sensors[i].timestamp = 0;
        _sensors[i].lastValidReading = 0;
        _sensors[i].sensorId = i;
        _sensors[i].address = "";
    }
}

bool TemperatureSensor::init() {
    DEBUG_PRINTLN("Initializing Temperature Sensors...");
    
    // Create OneWire instance
    _oneWire = new OneWire(DS18B20_PIN);
    
    // Create DallasTemperature instance
    _dallas = new DallasTemperature(_oneWire);
    
    // Start the library
    _dallas->begin();
    
    // Set resolution to 12 bits for highest accuracy
    _dallas->setResolution(12);
    
    // Scan for sensors
    scanSensors();
    
    if (_sensorCount == 0) {
        DEBUG_PRINTLN("WARNING: No temperature sensors found!");
        return false;
    }
    
    DEBUG_PRINTF("Found %d temperature sensor(s)\n", _sensorCount);
    
    // Perform initial reading
    update();
    
    return true;
}

void TemperatureSensor::scanSensors() {
    _sensorCount = 0;
    
    // Get device count
    uint8_t deviceCount = _dallas->getDeviceCount();
    
    DEBUG_PRINTF("Scanning for DS18B20 sensors... Found %d device(s)\n", deviceCount);
    
    // Iterate through devices
    for (uint8_t i = 0; i < deviceCount && i < MAX_SENSORS; i++) {
        if (_dallas->getAddress(_addresses[i], i)) {
            _sensors[i].address = addressToString(_addresses[i]);
            _sensors[i].sensorId = i;
            _sensorCount++;
            
            DEBUG_PRINTF("Sensor %d: %s\n", i, _sensors[i].address.c_str());
        }
    }
}

void TemperatureSensor::update() {
    // Check if enough time has passed since last reading
    if (millis() - _lastReadTime < TEMP_READ_INTERVAL) {
        return;
    }
    
    // Request temperatures from all sensors
    _dallas->requestTemperatures();
    
    // Read each sensor
    for (uint8_t i = 0; i < _sensorCount; i++) {
        float temp = _dallas->getTempC(_addresses[i]);
        
        if (isValidReading(temp)) {
            _sensors[i].temperature = temp;
            _sensors[i].valid = true;
            _sensors[i].timestamp = millis();
            _sensors[i].lastValidReading = millis();
        } else {
            // Check for timeout
            if (millis() - _sensors[i].lastValidReading > SENSOR_TIMEOUT) {
                _sensors[i].valid = false;
            }
        }
    }
    
    _lastReadTime = millis();
}

SensorData TemperatureSensor::getSensorData(uint8_t index) const {
    if (index < _sensorCount) {
        return _sensors[index];
    }
    
    // Return invalid sensor data
    SensorData invalid;
    invalid.valid = false;
    return invalid;
}

float TemperatureSensor::getAverageTemperature() const {
    float sum = 0;
    uint8_t validCount = 0;
    
    for (uint8_t i = 0; i < _sensorCount; i++) {
        if (_sensors[i].valid) {
            sum += _sensors[i].temperature;
            validCount++;
        }
    }
    
    if (validCount > 0) {
        return sum / validCount;
    }
    
    return 0.0;
}

bool TemperatureSensor::hasValidData() const {
    for (uint8_t i = 0; i < _sensorCount; i++) {
        if (_sensors[i].valid) {
            return true;
        }
    }
    return false;
}

String TemperatureSensor::addressToString(DeviceAddress addr) {
    String str = "";
    for (uint8_t i = 0; i < 8; i++) {
        if (addr[i] < 16) str += "0";
        str += String(addr[i], HEX);
        if (i < 7) str += ":";
    }
    return str;
}

bool TemperatureSensor::isValidReading(float temp) {
    // DS18B20 returns -127 for errors
    if (temp == DEVICE_DISCONNECTED_C || temp == -127.0) {
        return false;
    }
    
    // Check for reasonable temperature range
    if (temp < -55.0 || temp > 125.0) {
        return false;
    }
    
    return true;
}