#include "relay_controller.h"
#include <Arduino.h>

RelayController* g_relay_controller = nullptr;

RelayController::RelayController(uint8_t address) : pcf(address), current_relay_states(0) {
    i2c_mutex = xSemaphoreCreateMutex();
}

bool RelayController::begin() {
    pcf.begin();
    
    // Initialize buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Test connection
    return isConnected();
}

void RelayController::setCompressorState(bool on) {
    writeRelayState(RELAY_PIN_COMPRESSOR, on);
    
    // Update global state
    xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
    if (on) {
        g_system_state.relay_states |= (1 << RELAY_PIN_COMPRESSOR);
    } else {
        g_system_state.relay_states &= ~(1 << RELAY_PIN_COMPRESSOR);
    }
    xSemaphoreGive(g_system_state.mutex);
}

void RelayController::setFanState(bool on) {
    writeRelayState(RELAY_PIN_EVAP_FAN, on);
    
    // Update global state
    xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
    if (on) {
        g_system_state.relay_states |= (1 << RELAY_PIN_EVAP_FAN);
    } else {
        g_system_state.relay_states &= ~(1 << RELAY_PIN_EVAP_FAN);
    }
    xSemaphoreGive(g_system_state.mutex);
}

void RelayController::setDefrostHotGasState(bool on) {
    writeRelayState(RELAY_PIN_DEFROST_HOTGAS, on);
}

void RelayController::setDefrostElectricState(bool on) {
    writeRelayState(RELAY_PIN_DEFROST_ELEC, on);
}

bool RelayController::getCompressorFeedback() {
    return readFeedback(FEEDBACK_PIN_COMPRESSOR);
}

bool RelayController::getFanFeedback() {
    return readFeedback(FEEDBACK_PIN_EVAP_FAN);
}

void RelayController::setBuzzer(bool on) {
    digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
}

bool RelayController::isConnected() {
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        bool connected = pcf.isConnected();
        xSemaphoreGive(i2c_mutex);
        return connected;
    }
    return false;
}

void RelayController::updateRelayStates() {
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        pcf.write8(current_relay_states);
        xSemaphoreGive(i2c_mutex);
    }
}

void RelayController::writeRelayState(uint8_t pin, bool state) {
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (state) {
            current_relay_states |= (1 << pin);
        } else {
            current_relay_states &= ~(1 << pin);
        }
        pcf.write8(current_relay_states);
        xSemaphoreGive(i2c_mutex);
    }
}

bool RelayController::readFeedback(uint8_t pin) {
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Critical quasi-bidirectional read logic for PCF8574
        pcf.write(pin, HIGH); // Prime pin for input
        delay(1); // Small delay for signal stabilization
        bool state = pcf.read(pin) == HIGH;
        xSemaphoreGive(i2c_mutex);
        return state;
    }
    return false;
}