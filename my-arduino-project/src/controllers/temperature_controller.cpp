#include "controllers/temperature_controller.h"

// Global controller instance
TemperatureController controller;

TemperatureController::TemperatureController() {
    // Initialize configuration with defaults
    _config.mode = MODE_AUTO;
    _config.targetTemp = DEFAULT_TARGET_TEMP;
    _config.tempHysteresis = TEMP_DEADBAND;
    _config.TargetTemperature = DEFAULT_TARGET_TEMP;
    _config.TemperatureHysteresis = TEMP_DEADBAND;
    _config.DefrostIntervalMilliseconds = _config.defrostInterval;
    _config.DefrostDurationMilliseconds = _config.defrostDuration;
    _config.fanSpeed = DEFAULT_FAN_SPEED;
    _config.buzzerEnabled = true;
    _config.loggingEnabled = true;
    _config.defrostInterval = 6 * 3600 * 1000; // 6 hours
    _config.defrostDuration = 30 * 60 * 1000;  // 30 minutes
    
    // Initialize state
    _state.status = STATUS_INITIALIZING;
    _state.currentTemp = 0.0;
    _state.averageTemp = 0.0;
    _state.heatingActive = false;
    _state.coolingActive = false;
    _state.defrostActive = false;
    _state.fanPWM = 0;
    _state.lastUpdate = 0;
    _state.lastDefrost = 0;
    _state.errorCode = 0;
    
    // PID parameters for freezer control
    _kp = 2.0;
    _ki = 0.5;
    _kd = 1.0;
    _pidLastError = 0;
    _pidIntegral = 0;
    
    _lastControlUpdate = 0;
    _defrostStartTime = 0;
    _lastCompressorChange = 0;
    _lastCoolingRequest = 0;
    _lastHeatingRequest = 0;
    _overTempSince = 0;
    _underTempSince = 0;
    _sensorMissingSince = 0;
    _rangeFaultSince = 0;
    _defrostOverrunSince = 0;
    _eventHead = 0;
    _eventCount = 0;
}

bool TemperatureController::init() {
    DEBUG_PRINTLN("Initializing Temperature Controller...");
    
    // Configure control pins
    pinMode(RELAY_HEAT_PIN, OUTPUT);
    pinMode(RELAY_COOL_PIN, OUTPUT);
    pinMode(FAN_PWM_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    
    // Configure PWM for fan control
    ledcSetup(1, 25000, 8); // Channel 1, 25kHz, 8-bit resolution
    ledcAttachPin(FAN_PWM_PIN, 1);
    
    // Initialize outputs to safe state
    digitalWrite(RELAY_HEAT_PIN, LOW);
    digitalWrite(RELAY_COOL_PIN, LOW);
    ledcWrite(1, 0);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Reset PID controller
    _pidLastError = 0;
    _pidIntegral = 0;
    
    _state.status = STATUS_IDLE;
    _state.lastDefrost = millis();
    
    DEBUG_PRINTLN("Temperature Controller initialized successfully");
    return true;
}

void TemperatureController::updateWithMultipleSensors(const SensorData sensors[], uint8_t count) {
    if (millis() - _lastControlUpdate < TEMP_UPDATE_INTERVAL) {
        return;
    }
    
    // Evaluate faults (non-blocking)
    evaluateFaults(sensors, count);
    // Alarm escalation derived from fault bits (over/under temperature)
    unsigned long nowAlarm = millis();
    bool overUnder = faultActive(FaultBit(FAULT_OVER_TEMPERATURE_BIT)) || faultActive(FaultBit(FAULT_UNDER_TEMPERATURE_BIT));
    if (overUnder) {
        if (!_state.alarmActive) {
            // Start grace timer
            if (_state.alarmSince == 0) _state.alarmSince = nowAlarm;
            if (nowAlarm - _state.alarmSince >= ALARM_TRIGGER_GRACE_MS) {
                _state.alarmActive = true;
                _state.alarmSilenced = false; // ensure audible path
        logEvent(0xA100); // Alarm start
            }
        }
    } else {
        // Clear alarm state if condition resolved
        _state.alarmActive = false;
        _state.alarmSilenced = false;
        _state.alarmSince = 0;
        _state.alarmSilenceUntil = 0;
    // Log resolution only if previously active
    }
    // Silence expiry
    if (_state.alarmSilenced && nowAlarm > _state.alarmSilenceUntil) {
        _state.alarmSilenced = false; // re-arm audible/visual pulse
    }
    
    // Calculate average temperature from all valid sensors
    float avgTemp = calculateAverageTemp(sensors, count);
    _state.averageTemp = avgTemp;
    _state.currentTemp = avgTemp;
    
    // Check for defrost cycle
    if (_config.mode != MODE_OFF && shouldStartDefrost()) {
        startDefrost();
    }
    
    // Update control based on mode
    switch (_config.mode) {
        case MODE_AUTO:
            if (_state.defrostActive) {
                updateDefrostMode();
            } else {
                updateAutoMode(avgTemp);
            }
            break;
        case MODE_MANUAL_HEAT:
        case MODE_MANUAL_COOL:
            updateManualMode();
            break;
        case MODE_DEFROST:
            updateDefrostMode();
            break;
        case MODE_OFF:
        default:
            activateHeating(false);
            activateCooling(false);
            activateFan(0);
            _state.status = STATUS_IDLE;
            break;
    }
    
    updateOutputs();
    clearResolvedFaults();
    _lastControlUpdate = millis();
}

void TemperatureController::update(const SensorData& sensor) {
    SensorData sensors[1] = {sensor};
    updateWithMultipleSensors(sensors, 1);
}

void TemperatureController::updateAutoMode(float avgTemp) {
    float error = _config.targetTemp - avgTemp;
    float hysteresis = _config.tempHysteresis;
    if (fabs(error) < hysteresis) {
        activateHeating(false);
        activateCooling(false);
        activateFan(0);
        _state.status = STATUS_IDLE;
        return;
    }
    if (error > 0) { // Need heating
        activateHeating(true);
        activateCooling(false);
        uint8_t fanSpeed = constrain((uint8_t)abs(calculatePID(error)), 30, 100);
        activateFan(fanSpeed);
        _state.status = STATUS_HEATING;
    } else { // Need cooling
        activateHeating(false);
        activateCooling(true);
        uint8_t fanSpeed = constrain((uint8_t)abs(calculatePID(error)), 30, 100);
        activateFan(fanSpeed);
        _state.status = STATUS_COOLING;
    }
}

void TemperatureController::updateManualMode() {
    if (_config.mode == MODE_MANUAL_HEAT) {
        activateHeating(true);
        activateCooling(false);
        activateFan(_config.fanSpeed);
        _state.status = STATUS_HEATING;
    } else if (_config.mode == MODE_MANUAL_COOL) {
        activateHeating(false);
        activateCooling(true);
        activateFan(_config.fanSpeed);
        _state.status = STATUS_COOLING;
    }
}

void TemperatureController::updateDefrostMode() {
    // During defrost: heater on, compressor off, fan on high
    activateHeating(true);
    activateCooling(false);
    activateFan(100);
    _state.status = STATUS_DEFROST;
    
    // Check if defrost duration has elapsed
    if (millis() - _defrostStartTime >= _config.defrostDuration) {
        stopDefrost();
    }
}

void TemperatureController::activateHeating(bool enable) {
    if (_state.heatingActive != enable) {
        _state.heatingActive = enable;
        DEBUG_PRINT("Heating: ");
        DEBUG_PRINTLN(enable ? "ON" : "OFF");
    }
}

void TemperatureController::activateCooling(bool enable) {
    if (_state.coolingActive != enable) {
        _state.coolingActive = enable;
        DEBUG_PRINT("Cooling: ");
        DEBUG_PRINTLN(enable ? "ON" : "OFF");
    }
}

void TemperatureController::activateFan(uint8_t speed) {
    speed = constrain(speed, 0, 100);
    if (_state.fanPWM != speed) {
        _state.fanPWM = speed;
        DEBUG_PRINT("Fan Speed: ");
        DEBUG_PRINTLN(speed);
    }
}

void TemperatureController::updateOutputs() {
    digitalWrite(RELAY_HEAT_PIN, _state.heatingActive ? HIGH : LOW);
    digitalWrite(RELAY_COOL_PIN, _state.coolingActive ? HIGH : LOW);
    ledcWrite(1, map(_state.fanPWM, 0, 100, 0, 255));
    // Buzzer: active only during alarmActive and not silenced
    if (_config.buzzerEnabled) {
        if (_state.alarmActive && !_state.alarmSilenced) {
            // Simple periodic beep pattern (250ms ON each second)
            if ((millis() / 1000) % 2 == 0) {
                digitalWrite(BUZZER_PIN, HIGH);
            } else {
                digitalWrite(BUZZER_PIN, LOW);
            }
        } else {
            digitalWrite(BUZZER_PIN, LOW);
        }
    }
}

void TemperatureController::silenceAlarm() {
    if (_state.alarmActive && !_state.alarmSilenced) {
        _state.alarmSilenced = true;
        _state.alarmSilenceUntil = millis() + ALARM_SILENCE_DURATION_MS;
        logEvent(0xA110); // Alarm silenced
    }
}

void TemperatureController::logEvent(uint16_t code) {
    _eventLog[_eventHead] = { millis(), _state.faultMask, code };
    _eventHead = (_eventHead + 1) % EVENT_LOG_SIZE;
    if (_eventCount < EVENT_LOG_SIZE) _eventCount++;
}

void TemperatureController::safetyCheck(const SensorData sensors[], uint8_t count) {
    // Check for sensor failures
    bool hasValidSensor = false;
    for (uint8_t i = 0; i < count; i++) {
        if (sensors[i].valid) {
            hasValidSensor = true;
            
            // Check for extreme temperatures
            if (sensors[i].temperature < TEMP_MIN_SAFE || 
                sensors[i].temperature > TEMP_MAX_SAFE) {
    updateFault(FaultBit(FAULT_SENSOR_RANGE_BIT), true);
    if (_rangeFaultSince == 0) _rangeFaultSince = millis();
        return; // continue non-blocking strategy (no STATUS_ERROR)
            }
        }
    }
    
    if (!hasValidSensor) {
    updateFault(FaultBit(FAULT_SENSOR_MISSING_BIT), true);
    if (_sensorMissingSince == 0) _sensorMissingSince = millis();
    // Disable outputs safely but do not mark STATUS_ERROR
    activateHeating(false);
    activateCooling(false);
    activateFan(0);
    }
}

void TemperatureController::setMode(SystemMode mode) {
    if (_config.mode != mode) {
        _config.mode = mode;
        
        // Reset PID when changing modes
        _pidLastError = 0;
        _pidIntegral = 0;
        
        DEBUG_PRINT("Mode changed to: ");
        DEBUG_PRINTLN(mode);
    }
}

void TemperatureController::setTargetTemperature(float temp) {
    temp = constrain(temp, TEMP_MIN_SAFE, TEMP_MAX_SAFE);
    _config.targetTemp = temp;
    _config.TargetTemperature = temp;
    DEBUG_PRINT("Target temperature set to: ");
    DEBUG_PRINTLN(temp);
}

void TemperatureController::setFanSpeed(uint8_t speed) {
    _config.fanSpeed = constrain(speed, 0, 100);
    DEBUG_PRINT("Fan speed set to: ");
    DEBUG_PRINTLN(_config.fanSpeed);
}

void TemperatureController::setHysteresis(float value) {
    _config.tempHysteresis = constrain(value, 0.5, 5.0);
    _config.TemperatureHysteresis = _config.tempHysteresis;
    DEBUG_PRINT("Hysteresis set to: ");
    DEBUG_PRINTLN(_config.tempHysteresis);
}

void TemperatureController::emergency_stop() {
    digitalWrite(RELAY_HEAT_PIN, LOW);
    digitalWrite(RELAY_COOL_PIN, LOW);
    ledcWrite(1, 0);
    // STATUS_ERROR retained only for truly unrecoverable conditions (not general faults)
    
    if (_config.buzzerEnabled) {
        // Sound alarm pattern
        for (int i = 0; i < 3; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(200);
            digitalWrite(BUZZER_PIN, LOW);
            delay(100);
        }
    }
    
    DEBUG_PRINTLN("EMERGENCY STOP ACTIVATED!");
}

void TemperatureController::startDefrost() {
    if (!_state.defrostActive) {
        _state.defrostActive = true;
        _defrostStartTime = millis();
        _state.lastDefrost = _defrostStartTime;
        DEBUG_PRINTLN("Defrost cycle started");
    }
}

void TemperatureController::stopDefrost() {
    if (_state.defrostActive) {
        _state.defrostActive = false;
        _defrostStartTime = 0;
        DEBUG_PRINTLN("Defrost cycle completed");
    }
}

float TemperatureController::calculatePID(float error) {
    float derivative = error - _pidLastError;
    _pidIntegral += error;
    
    // Anti-windup
    _pidIntegral = constrain(_pidIntegral, -100, 100);
    
    float output = (_kp * error) + (_ki * _pidIntegral) + (_kd * derivative);
    
    _pidLastError = error;
    
    return constrain(output, -100, 100);
}

float TemperatureController::calculateAverageTemp(const SensorData sensors[], uint8_t count) {
    float sum = 0;
    uint8_t validCount = 0;
    
    for (uint8_t i = 0; i < count; i++) {
        if (sensors[i].valid) {
            sum += sensors[i].temperature;
            validCount++;
        }
    }
    
    if (validCount > 0) {
        return sum / validCount;
    }
    
    return 0.0;
}

bool TemperatureController::shouldStartDefrost() {
    // Check if enough time has passed since last defrost
    if (millis() - _state.lastDefrost >= _config.defrostInterval) {
        return true;
    }
    
    // Could add additional conditions here (e.g., ice sensor, efficiency drop)
    return false;
}

void TemperatureController::updateFault(uint32_t bit, bool active) {
    if (active) {
        _state.faultMask |= bit;
    } else {
        _state.faultMask &= ~bit;
    }
}

void TemperatureController::clearAllFaults() {
    _state.faultMask = 0;
    _state.errorCode = 0;
    if (_state.status == STATUS_ERROR) {
        _state.status = STATUS_IDLE;
    }
}

bool TemperatureController::hasValidSensor(const SensorData sensors[], uint8_t count) const {
    for (uint8_t i = 0; i < count; ++i) if (sensors[i].valid) return true;
    return false;
}

void TemperatureController::evaluateFaults(const SensorData sensors[], uint8_t count) {
    unsigned long now = millis();
    bool anyValid = false;
    bool rangeFault = false;
    float avg = calculateAverageTemp(sensors, count);
    float overThresh = _config.targetTemp + OVER_TEMPERATURE_MARGIN;
    float underThresh = _config.targetTemp - UNDER_TEMPERATURE_MARGIN;

    // Sensor presence & range
    for (uint8_t i = 0; i < count; ++i) {
        if (sensors[i].valid) {
            anyValid = true;
            if (sensors[i].temperature < TEMP_MIN_SAFE || sensors[i].temperature > TEMP_MAX_SAFE) {
                rangeFault = true;
            }
        }
    }
    if (!anyValid) {
        if (_sensorMissingSince == 0) _sensorMissingSince = now;
        if (now - _sensorMissingSince >= SENSOR_MISSING_DEBOUNCE_MS) updateFault(FaultBit(FAULT_SENSOR_MISSING_BIT), true);
    } else {
        _sensorMissingSince = 0; // resolution candidate; clearing handled in clearResolvedFaults
    }
    if (rangeFault) {
        if (_rangeFaultSince == 0) _rangeFaultSince = now;
        if (now - _rangeFaultSince >= RANGE_FAULT_DEBOUNCE_MS) updateFault(FaultBit(FAULT_SENSOR_RANGE_BIT), true);
    } else {
        _rangeFaultSince = 0;
    }

    // Over / Under temperature (relative to target)
    if (anyValid) {
        if (avg > overThresh) {
            if (_overTempSince == 0) _overTempSince = now;
            if (now - _overTempSince >= FAULT_DEBOUNCE_MS) updateFault(FaultBit(FAULT_OVER_TEMPERATURE_BIT), true);
        } else {
            _overTempSince = 0;
        }
        if (avg < underThresh) {
            if (_underTempSince == 0) _underTempSince = now;
            if (now - _underTempSince >= FAULT_DEBOUNCE_MS) updateFault(FaultBit(FAULT_UNDER_TEMPERATURE_BIT), true);
        } else {
            _underTempSince = 0;
        }
    }

    // Defrost timeout fault
    if (_state.defrostActive) {
        if (now - _defrostStartTime > (_config.defrostDuration + DEFROST_TIMEOUT_GRACE_MS)) {
            if (_defrostOverrunSince == 0) _defrostOverrunSince = now;
            if (now - _defrostOverrunSince >= FAULT_DEBOUNCE_MS) updateFault(FaultBit(FAULT_DEFROST_TIMEOUT_BIT), true);
        }
    } else {
        _defrostOverrunSince = 0;
    }

    // Short cycle detection (cooling relay gating) – set fault if request arrives before minimum off time elapsed
    // Implemented via gateShortCycle called when cooling activation desired (see updateAutoMode modifications below if extended)
}

void TemperatureController::clearResolvedFaults() {
    unsigned long now = millis();
    // Clear sensor missing when we have a valid sensor for debounce period
    if (faultActive(FaultBit(FAULT_SENSOR_MISSING_BIT))) {
        if (_sensorMissingSince == 0 && hasValidSensor(nullptr, 0) == false) {
            // cannot evaluate without sensors list; leave as is (will be cleared next evaluation when integrated with sensor pass)
        }
    }
    // Clear range fault when no range fault flagged this cycle
    if (!faultActive(FaultBit(FAULT_SENSOR_RANGE_BIT)) || _rangeFaultSince == 0) {
        // (Range fault clearance is handled indirectly by evaluateFaults resetting _rangeFaultSince)
    }
    // Over / Under temperature self-clear when back inside +/- hysteresis band of target for debounce period (reuse timers)
    float upperClear = _config.targetTemp + (_config.tempHysteresis * 0.5f);
    float lowerClear = _config.targetTemp - (_config.tempHysteresis * 0.5f);
    if (faultActive(FaultBit(FAULT_OVER_TEMPERATURE_BIT)) && _state.currentTemp <= upperClear) {
        updateFault(FaultBit(FAULT_OVER_TEMPERATURE_BIT), false);
    }
    if (faultActive(FaultBit(FAULT_UNDER_TEMPERATURE_BIT)) && _state.currentTemp >= lowerClear) {
        updateFault(FaultBit(FAULT_UNDER_TEMPERATURE_BIT), false);
    }
    // Defrost timeout fault clears when a new successful defrost cycle starts and completes under allowed duration
    if (faultActive(FaultBit(FAULT_DEFROST_TIMEOUT_BIT)) && !_state.defrostActive && _defrostOverrunSince == 0) {
        // heuristic clear after cycle ended and no overrun timer
        updateFault(FaultBit(FAULT_DEFROST_TIMEOUT_BIT), false);
    }
}

void TemperatureController::gateShortCycle(bool wantCooling) {
    unsigned long now = millis();
    if (wantCooling) {
        _lastCoolingRequest = now;
        if (!_state.coolingActive) {
            // Check minimum off time
            if (now - _lastCompressorChange < MIN_COMPRESSOR_OFF_TIME_MS) {
                updateFault(FaultBit(FAULT_COMPRESSOR_SHORT_CYCLE_BIT), true);
                // Do not enable cooling yet
                return;
            } else {
                updateFault(FaultBit(FAULT_COMPRESSOR_SHORT_CYCLE_BIT), false);
                activateCooling(true);
                _lastCompressorChange = now;
            }
        }
    } else {
        if (_state.coolingActive) {
            // Respect minimum on time
            if (now - _lastCompressorChange < MIN_COMPRESSOR_ON_TIME_MS) {
                return; // keep running until min on time met
            }
            activateCooling(false);
            _lastCompressorChange = now;
        }
    }
}