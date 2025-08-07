#include "sensor_manager.h"

SensorManager& SensorManager::getInstance() {
    static SensorManager instance;
    return instance;
}

bool SensorManager::init() {
    DEBUG_PRINTLN("Initializing Sensor Manager...");
    
    // Initialize OneWire and DallasTemperature
    oneWire.begin(DS18B20_PIN);
    sensors.setOneWire(&oneWire);
    sensors.begin();
    
    // Initialize arrays
    for (int i = 0; i < 4; i++) {
        sensor_offsets[i] = 0.0f;
        sensors_found[i] = false;
        sensor_fault_count[i] = 0;
        last_valid_reading_time[i] = 0;
    }
    
    // Discover connected sensors
    if (!discoverSensors()) {
        DEBUG_PRINTLN("Warning: Not all sensors found during discovery");
    }
    
    // Set resolution to 12-bit for best accuracy
    sensors.setResolution(12);
    
    // Set conversion time (750ms for 12-bit resolution)
    sensors.setWaitForConversion(true);
    
    DEBUG_PRINTF("Sensor Manager initialized with %d sensors found\n", num_sensors_found);
    return true;
}

bool SensorManager::discoverSensors() {
    num_sensors_found = sensors.getDeviceCount();
    DEBUG_PRINTF("Found %d sensors on 1-Wire bus\n", num_sensors_found);
    
    if (num_sensors_found == 0) {
        DEBUG_PRINTLN("Error: No sensors found on 1-Wire bus");
        return false;
    }
    
    // Get addresses of all found sensors
    for (uint8_t i = 0; i < min(num_sensors_found, 4); i++) {
        if (sensors.getAddress(sensor_addresses[i], i)) {
            sensors_found[i] = true;
            DEBUG_PRINTF("Sensor %d address: ", i);
            for (uint8_t j = 0; j < 8; j++) {
                DEBUG_PRINTF("%02X", sensor_addresses[i][j]);
            }
            DEBUG_PRINTLN();
        } else {
            DEBUG_PRINTF("Unable to get address for sensor %d\n", i);
        }
    }
    
    return (num_sensors_found >= 1);  // At least cabin sensor required
}

void SensorManager::readAllSensors() {
    // Request temperatures from all sensors
    sensors.requestTemperatures();
    
    // Read each sensor with error checking and store in local variables
    float cabin_temp = INVALID_TEMP;
    float evap_temp = INVALID_TEMP;
    float condenser_temp = INVALID_TEMP;
    float suction_temp = INVALID_TEMP;
    bool sensor_valid[4] = {false, false, false, false};
    
    for (uint8_t i = 0; i < 4; i++) {
        SensorIndex sensor = static_cast<SensorIndex>(i);
        float temp = -999.0f;
        
        if (sensors_found[i]) {
            temp = sensors.getTempC(sensor_addresses[i]);
        } else {
            // Sensor not discovered, mark as invalid
            temp = INVALID_TEMP;
        }
        
        // Apply calibration offset
        if (isTemperatureValid(temp)) {
            temp += sensor_offsets[i];
        }
        
        // Store in local variables
        switch (sensor) {
            case SensorIndex::CABIN:
                cabin_temp = temp;
                break;
            case SensorIndex::EVAPORATOR:
                evap_temp = temp;
                break;
            case SensorIndex::CONDENSER:
                condenser_temp = temp;
                break;
            case SensorIndex::SUCTION:
                suction_temp = temp;
                break;
        }
        
        // Update sensor status
        updateSensorStatus(sensor, temp);
        sensor_valid[i] = isSensorValid(sensor);
    }
    
    // Update global state with mutex protection
    if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_state.sensors.cabin_temp_c = cabin_temp;
        g_state.sensors.evap_temp_c = evap_temp;
        g_state.sensors.condenser_temp_c = condenser_temp;
        g_state.sensors.suction_temp_c = suction_temp;
        
        for (int i = 0; i < 4; i++) {
            g_state.sensors.sensor_valid[i] = sensor_valid[i];
        }
        
        g_state.sensors.last_read_ms = millis();
        g_state.hmi_needs_update = true;
        
        xSemaphoreGive(g_state.mutex);
    }
}

bool SensorManager::isTemperatureValid(float temp) {
    return (temp != INVALID_TEMP && temp != SHORT_CIRCUIT_TEMP && 
            temp > -50.0f && temp < 100.0f);
}

void SensorManager::updateSensorStatus(SensorIndex sensor, float temperature) {
    uint8_t idx = static_cast<uint8_t>(sensor);
    
    if (isTemperatureValid(temperature)) {
        // Valid reading - reset fault count and update timestamp
        sensor_fault_count[idx] = 0;
        last_valid_reading_time[idx] = millis();
    } else {
        // Invalid reading - increment fault count
        sensor_fault_count[idx]++;
        
        // Log the specific error
        if (temperature == INVALID_TEMP) {
            DEBUG_PRINTF("Sensor %s: Open circuit detected\n", getSensorName(sensor));
        } else if (temperature == SHORT_CIRCUIT_TEMP) {
            DEBUG_PRINTF("Sensor %s: Short circuit detected\n", getSensorName(sensor));
        } else {
            DEBUG_PRINTF("Sensor %s: Out of range reading: %.2f\n", getSensorName(sensor), temperature);
        }
    }
}

bool SensorManager::isSensorValid(SensorIndex sensor) {
    uint8_t idx = static_cast<uint8_t>(sensor);
    uint32_t current_time = millis();
    
    // Sensor is valid if:
    // 1. It was discovered during init
    // 2. Fault count is below threshold
    // 3. We've had a valid reading recently
    return (sensors_found[idx] && 
            sensor_fault_count[idx] < MAX_FAULT_COUNT &&
            (current_time - last_valid_reading_time[idx]) < SENSOR_TIMEOUT_MS);
}

float SensorManager::getTemperature(SensorIndex sensor) {
    float temp = INVALID_TEMP;
    
    if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        switch (sensor) {
            case SensorIndex::CABIN:
                temp = g_state.sensors.cabin_temp_c;
                break;
            case SensorIndex::EVAPORATOR:
                temp = g_state.sensors.evap_temp_c;
                break;
            case SensorIndex::CONDENSER:
                temp = g_state.sensors.condenser_temp_c;
                break;
            case SensorIndex::SUCTION:
                temp = g_state.sensors.suction_temp_c;
                break;
            default:
                temp = INVALID_TEMP;
                break;
        }
        xSemaphoreGive(g_state.mutex);
    }
    
    return temp;
}

const char* SensorManager::getSensorName(SensorIndex sensor) {
    switch (sensor) {
        case SensorIndex::CABIN: return "Cabin";
        case SensorIndex::EVAPORATOR: return "Evaporator";
        case SensorIndex::CONDENSER: return "Condenser";
        case SensorIndex::SUCTION: return "Suction";
        default: return "Unknown";
    }
}

void SensorManager::setSensorOffset(SensorIndex sensor, float offset) {
    uint8_t idx = static_cast<uint8_t>(sensor);
    if (idx < 4) {
        sensor_offsets[idx] = constrain(offset, -10.0f, 10.0f);  // Limit offset range
        DEBUG_PRINTF("Set %s sensor offset to %.2f°C\n", getSensorName(sensor), offset);
    }
}

float SensorManager::getSensorOffset(SensorIndex sensor) {
    uint8_t idx = static_cast<uint8_t>(sensor);
    return (idx < 4) ? sensor_offsets[idx] : 0.0f;
}

bool SensorManager::checkSensorFaults() {
    FaultCode highest_fault = getHighestPriorityFault();
    
    // Update global state with highest priority fault
    if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Only update if this is a higher priority fault than current
        if (static_cast<uint8_t>(highest_fault) > static_cast<uint8_t>(g_state.status.active_fault) ||
            g_state.status.active_fault == FaultCode::NONE) {
            g_state.status.active_fault = highest_fault;
            g_state.hmi_needs_update = true;
        }
        xSemaphoreGive(g_state.mutex);
    }
    
    return (highest_fault != FaultCode::NONE);
}

FaultCode SensorManager::getHighestPriorityFault() {
    // Check sensors in priority order (cabin sensor is most critical)
    
    // Cabin sensor faults (highest priority)
    if (!isSensorValid(SensorIndex::CABIN)) {
        return getSensorFaultCode(SensorIndex::CABIN);
    }
    
    // Other sensor faults (lower priority)
    if (!isSensorValid(SensorIndex::EVAPORATOR)) {
        return FaultCode::EVAP_SENSOR_FAIL;
    }
    
    if (!isSensorValid(SensorIndex::CONDENSER)) {
        return FaultCode::CONDENSER_SENSOR_FAIL;
    }
    
    if (!isSensorValid(SensorIndex::SUCTION)) {
        return FaultCode::SUCTION_SENSOR_FAIL;
    }
    
    return FaultCode::NONE;
}

FaultCode SensorManager::getSensorFaultCode(SensorIndex sensor) {
    if (sensor == SensorIndex::CABIN) {
        // For cabin sensor, distinguish between open and short
        uint8_t idx = static_cast<uint8_t>(sensor);
        float last_temp = getTemperature(sensor);
        
        if (last_temp == SHORT_CIRCUIT_TEMP) {
            return FaultCode::CABIN_SENSOR_SHORT;
        } else {
            return FaultCode::CABIN_SENSOR_OPEN;
        }
    }
    
    // For other sensors, use generic fault codes
    switch (sensor) {
        case SensorIndex::EVAPORATOR:
            return FaultCode::EVAP_SENSOR_FAIL;
        case SensorIndex::CONDENSER:
            return FaultCode::CONDENSER_SENSOR_FAIL;
        case SensorIndex::SUCTION:
            return FaultCode::SUCTION_SENSOR_FAIL;
        default:
            return FaultCode::NONE;
    }
}