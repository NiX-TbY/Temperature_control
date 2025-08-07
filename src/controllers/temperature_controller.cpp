#include <Arduino.h>
#include "controllers/temperature_controller.h"
#include "types/types.h"
#include "config/config.h"

// Global controller instance
TemperatureController controller;

TemperatureController::TemperatureController() {
    _state = ControlState();
    _config = ControlConfig();
    _lastControlUpdate = 0;
    _pidLastError = 0.0;
    _pidIntegral = 0.0;
    _kp = 1.0;
    _ki = 0.1;
    _kd = 0.05;
    invalidDataStart = 0;
}

bool TemperatureController::init() {
    // Initialize pins
    pinMode(RELAY_HEAT_PIN, OUTPUT);
    pinMode(RELAY_COOL_PIN, OUTPUT);
    pinMode(FAN_PWM_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    
    // Initialize outputs to safe state
    digitalWrite(RELAY_HEAT_PIN, LOW);
    digitalWrite(RELAY_COOL_PIN, LOW);
    analogWrite(FAN_PWM_PIN, 0);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Set initial state
    _state.status = STATUS_IDLE;
    _state.heatingActive = false;
    _state.coolingActive = false;
    _state.fanPWM = 0;
    _state.lastStateChange = millis();
    
    DEBUG_PRINTLN("TemperatureController initialized");
    return true;
}

void TemperatureController::update(const SensorData& sensor) {
    if (millis() - _lastControlUpdate < TEMP_UPDATE_INTERVAL) {
        return;
    }
    
    // Perform safety checks first
    safetyCheck(sensor);
    
    if (_state.status == STATUS_ERROR) {
        emergency_stop();
        return;
    }
    
    // Execute control logic based on mode
    switch (_config.mode) {
        case MODE_AUTO:
            updateAutoMode(sensor);
            break;
        
        case MODE_MANUAL:
        case MODE_HEAT:
        case MODE_COOL:
        case MODE_FAN_ONLY:
            updateManualMode(sensor);
            break;
            
        case MODE_OFF:
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
    if (!sensor.isValid) {
        return;
    }
    
    float error = _config.targetTemp - sensor.temperature;
    
    // Simple hysteresis control for now
    if (error > TEMP_HYSTERESIS_C) {
        // Need heating
        activateHeating(true);
        activateCooling(false);
        activateFan(_config.fanSpeed);
        _state.status = STATUS_HEATING;
    }
    else if (error < -TEMP_HYSTERESIS_C) {
        // Need cooling
        activateHeating(false);
        activateCooling(true);
        activateFan(_config.fanSpeed);
        _state.status = STATUS_COOLING;
    }
    else {
        // Within setpoint range
        activateHeating(false);
        activateCooling(false);
        activateFan(0);
        _state.status = STATUS_IDLE;
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
        default:
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
    if (_state.fanPWM != speed) {
        _state.fanPWM = speed;
        _state.lastStateChange = millis();
        DEBUG_PRINTF("Fan speed set to %d\n", speed);
    }
}

void TemperatureController::updateOutputs() {
    digitalWrite(RELAY_HEAT_PIN, _state.heatingActive ? HIGH : LOW);
    digitalWrite(RELAY_COOL_PIN, _state.coolingActive ? HIGH : LOW);
    analogWrite(FAN_PWM_PIN, map(_state.fanPWM, 0, 100, 0, 255));
}

void TemperatureController::safetyCheck(const SensorData& sensor) {
    if (!sensor.isValid) {
        if (invalidDataStart == 0) {
            invalidDataStart = millis();
        } else if (millis() - invalidDataStart > 30000) { // 30 seconds of invalid data
            _state.status = STATUS_ERROR;
            DEBUG_PRINTLN("ERROR: Sensor data invalid for too long");
            return;
        }
    } else {
        invalidDataStart = 0;
        if (sensor.temperature < MIN_TEMP || sensor.temperature > MAX_TEMP) {
            _state.status = STATUS_ERROR;
            DEBUG_PRINTF("ERROR: Temperature out of range: %.1f°C\n", sensor.temperature);
            return;
        }
    }
}

void TemperatureController::setMode(SystemMode mode) {
    _config.mode = mode;
    DEBUG_PRINTF("Mode set to: %d\n", mode);
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
        for (int i = 0; i < 3; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(200);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
        }
    }
    
    DEBUG_PRINTLN("EMERGENCY STOP ACTIVATED");
}

void TemperatureController::setFanSpeed(uint8_t speed) {
    _config.fanSpeed = constrain(speed, 0, 100);
    DEBUG_PRINTF("Fan speed set to: %d%%\n", _config.fanSpeed);
}

float TemperatureController::calculatePID(float error) {
    float derivative = error - _pidLastError;
    _pidIntegral += error;
    
    // Prevent integral windup
    _pidIntegral = constrain(_pidIntegral, -100.0f, 100.0f);
    
    float output = (_kp * error) + (_ki * _pidIntegral) + (_kd * derivative);
    
    _pidLastError = error;
    
    return output;
}