#include "io_controller.h"
#include "config.h"
#include <Arduino.h>

RelayController::RelayController(uint8_t address) : pcf(address), current_relay_states(0) {}

bool RelayController::begin() {
    if (!pcf.begin()) {
        Serial.println("ERROR: PCF8574 not found at specified address!");
        return false;
    }
    
    // Initialize all pins as outputs and set to LOW
    pcf.write8(0x00);
    current_relay_states = 0x00;
    
    // Configure buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    Serial.println("Relay Controller initialized successfully");
    return true;
}

bool RelayController::isConnected() {
    return pcf.isConnected();
}

void RelayController::setCompressorState(bool on) {
    if (on) {
        current_relay_states |= (1 << RELAY_PIN_COMPRESSOR);
        Serial.println("Compressor relay ON");
    } else {
        current_relay_states &= ~(1 << RELAY_PIN_COMPRESSOR);
        Serial.println("Compressor relay OFF");
    }
    pcf.write8(current_relay_states);
}

void RelayController::setFanState(bool on) {
    if (on) {
        current_relay_states |= (1 << RELAY_PIN_EVAP_FAN);
        Serial.println("Evaporator fan relay ON");
    } else {
        current_relay_states &= ~(1 << RELAY_PIN_EVAP_FAN);
        Serial.println("Evaporator fan relay OFF");
    }
    pcf.write8(current_relay_states);
}

void RelayController::setDefrostState(bool on) {
    if (on) {
        current_relay_states |= (1 << RELAY_PIN_DEFROST_HOTGAS);
        Serial.println("Defrost relay ON");
    } else {
        current_relay_states &= ~(1 << RELAY_PIN_DEFROST_HOTGAS);
        Serial.println("Defrost relay OFF");
    }
    pcf.write8(current_relay_states);
}

bool RelayController::getCompressorFeedback() {
    // Critical quasi-bidirectional read logic for PCF8574
    // Must write HIGH first to enable input mode on the pin
    pcf.write(FEEDBACK_PIN_COMPRESSOR, HIGH);
    delay(1); // Small delay to allow pin to settle
    bool feedback = pcf.read(FEEDBACK_PIN_COMPRESSOR);
    return feedback;
}

bool RelayController::getFanFeedback() {
    // Critical quasi-bidirectional read logic for PCF8574
    pcf.write(FEEDBACK_PIN_EVAP_FAN, HIGH);
    delay(1);
    bool feedback = pcf.read(FEEDBACK_PIN_EVAP_FAN);
    return feedback;
}

void RelayController::setBuzzer(bool on) {
    digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
    if (on) {
        Serial.println("Buzzer ON");
    } else {
        Serial.println("Buzzer OFF");
    }
}