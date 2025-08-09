#include "controllers/temperature_controller.h"

TemperatureController::TemperatureController() {
    _config.targetTemp = DEFAULT_TARGET_TEMP;
    _config.mode = MODE_OFF;
    _config.tempTolerance = TEMP_TOLERANCE;
    _config.fanSpeed = 50;
}

bool TemperatureController::init() {
    DEBUG_PRINTLN("Initializing temperature controller...");
    
    // Initialize relay pins
    pinMode(RELAY_HEAT_PIN, OUTPUT);
    pinMode(RELAY_COOL_PIN, OUTPUT);
    pinMode(FAN_PWM_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    
    // Ensure all outputs are off
    digitalWrite(RELAY_HEAT_PIN, LOW);
    digitalWrite(RELAY_COOL_PIN, LOW);
    analogWrite(FAN_PWM_PIN, 0);
    digitalWrite(BUZZER_PIN, LOW);
    
    _state.status = STATUS_IDLE;
    
    DEBUG_PRINTLN("Temperature controller initialized");
    return true;
}

void TemperatureController::update(const SensorData& sensor) {
    if (millis() - _lastControlUpdate < TEMP_UPDATE_INTERVAL) {
        return;
    }
    
    // Safety check first
    safetyCheck(sensor);
    
    if (_state.status == STATUS_ERROR) {
        emergency_stop();
        return;
    }
    
    // Update control logic based on mode
    switch (_config.mode) {
        case MODE_AUTO:
            updateAutoMode(sensor);
            break;
        case MODE_HEAT:
        case MODE_COOL:
        case MODE_FAN_ONLY:
            updateManualMode(sensor);
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
    _lastControlUpdate = millis();
}

void TemperatureController::updateAutoMode(const SensorData& sensor) {
    if (!sensor.isValid) return;
    
    float error = _config.targetTemp - sensor.temperature;
    float absError = abs(error);
    
    if (absError <= _config.tempTolerance) {
        // Within tolerance - idle state
        activateHeating(false);
        activateCooling(false);
        activateFan(20); // Low fan for circulation
        _state.status = STATUS_IDLE;
    } else if (error > 0) {
        // Need heating
        activateHeating(true);
        activateCooling(false);
        activateFan(_config.fanSpeed);
        _state.status = STATUS_HEATING;
    } else {
        // Need cooling
        activateHeating(false);
        activateCooling(true);
        activateFan(_config.fanSpeed);
        _state.status = STATUS_COOLING;
    }
}

void TemperatureController::updateManualMode(const SensorData& sensor) {
    switch (_config.mode) {
        case MODE_HEAT:
            activateHeating(true);
            activateCooling(false);
            activateFan(_config.fanSpeed);
            _state.status = STATUS_HEATING;
            break;
        case MODE_COOL:
            activateHeating(false);
            activateCooling(true);
            activateFan(_config.fanSpeed);
            _state.status = STATUS_COOLING;
            break;
        case MODE_FAN_ONLY:
            activateHeating(false);
            activateCooling(false);
            activateFan(_config.fanSpeed);
            _state.status = STATUS_FAN_RUNNING;
            break;
    }
}

void TemperatureController::activateHeating(bool enable) {
    if (_state.heatingActive != enable) {
        _state.heatingActive = enable;
        _state.lastStateChange = millis();
        DEBUG_PRINTF("Heating %s\n", enable ? "ON" : "OFF");
    }
}

void TemperatureController::activateCooling(bool enable) {
    if (_state.coolingActive != enable) {
        _state.coolingActive = enable;
        _state.lastStateChange = millis();
        DEBUG_PRINTF("Cooling %s\n", enable ? "ON" : "OFF");
    }
}

void TemperatureController::activateFan(uint8_t speed) {
    speed = constrain(speed, 0, 100);
    if (_state.fanPWM != speed) {
        _state.fanPWM = speed;
        _state.fanActive = (speed > 0);
        DEBUG_PRINTF("Fan speed set to %d%%\n", speed);
    }
}

void TemperatureController::updateOutputs() {
    digitalWrite(RELAY_HEAT_PIN, _state.heatingActive ? HIGH : LOW);
    digitalWrite(RELAY_COOL_PIN, _state.coolingActive ? HIGH : LOW);
    analogWrite(FAN_PWM_PIN, map(_state.fanPWM, 0, 100, 0, 255));
}

void TemperatureController::safetyCheck(const SensorData& sensor) {
    if (!sensor.isValid) {
        static unsigned long invalidDataStart = 0;
        if (invalidDataStart == 0) {
            invalidDataStart = millis();
        } else if (millis() - invalidDataStart > 30000) { // 30 seconds of invalid data
            _state.status = STATUS_ERROR;
            DEBUG_PRINTLN("ERROR: Sensor data invalid for too long");
        }
    } else {
        // Check temperature limits
        if (sensor.temperature < MIN_TEMP || sensor.temperature > MAX_TEMP) {
            _state.status = STATUS_ERROR;
            DEBUG_PRINTF("ERROR: Temperature out of range: %.1f°C\n", sensor.temperature);
        }
    }
}

void TemperatureController::setMode(SystemMode mode) {
    _config.mode = mode;
    DEBUG_PRINTF("Mode changed to: %d\n", mode);
}

void TemperatureController::setTargetTemperature(float temp) {
    temp = constrain(temp, MIN_TEMP, MAX_TEMP);
    _config.targetTemp = temp;
    DEBUG_PRINTF("Target temperature set to: %.1f°C\n", temp);
}

void TemperatureController::emergency_stop() {
    activateHeating(false);
    activateCooling(false);
    activateFan(0);
    _state.status = STATUS_ERROR;
    
    if (_config.buzzerEnabled) {
        // Sound alarm
        for (int i = 0; i < 3; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(200);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
        }
    }
    
    DEBUG_PRINTLN("EMERGENCY STOP ACTIVATED");
}

TemperatureController controller;