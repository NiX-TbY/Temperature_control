#include "relay_controller.h"

RelayController& RelayController::getInstance() {
    static RelayController instance;
    return instance;
}

bool RelayController::init() {
    DEBUG_PRINTLN("Initializing Relay Controller...");
    
    // Initialize PCF8574 with I2C address
    pcf8574.begin(PCF8574_I2C_ADDR);
    
    // Initialize state tracking
    current_relay_states = 0x00;  // All relays off
    compressor_feedback_fault_start = 0;
    fan_feedback_fault_start = 0;
    compressor_feedback_fault_active = false;
    fan_feedback_fault_active = false;
    
    // Check if PCF8574 is connected
    if (!isConnected()) {
        DEBUG_PRINTLN("Error: PCF8574 not responding");
        return false;
    }
    
    // Set all relay pins as outputs (LOW = relay off)
    pcf8574.write8(0x00);
    
    // Configure feedback pins for input by writing HIGH to them
    // This is required for the PCF8574's quasi-bidirectional operation
    pcf8574.write(FEEDBACK_COMPRESSOR_PIN, HIGH);
    pcf8574.write(FEEDBACK_EVAP_FAN_PIN, HIGH);
    
    // Initialize buzzer GPIO
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    DEBUG_PRINTLN("Relay Controller initialized successfully");
    return true;
}

bool RelayController::isConnected() {
    return pcf8574.isConnected();
}

void RelayController::setCompressor(bool state) {
    updateRelayState(RELAY_COMPRESSOR_PIN, state);
    
    // Update global state
    SAFE_STATE_WRITE(status.compressor_running, state);
    
    DEBUG_PRINTF("Compressor set to %s\n", state ? "ON" : "OFF");
}

void RelayController::setEvaporatorFan(bool state) {
    updateRelayState(RELAY_EVAP_FAN_PIN, state);
    
    // Update global state
    SAFE_STATE_WRITE(status.evap_fan_running, state);
    
    DEBUG_PRINTF("Evaporator fan set to %s\n", state ? "ON" : "OFF");
}

void RelayController::setDefrostHotGas(bool state) {
    updateRelayState(RELAY_DEFROST_HOTGAS_PIN, state);
    DEBUG_PRINTF("Hot gas defrost set to %s\n", state ? "ON" : "OFF");
}

void RelayController::setDefrostElectric(bool state) {
    updateRelayState(RELAY_DEFROST_ELEC_PIN, state);
    DEBUG_PRINTF("Electric defrost set to %s\n", state ? "ON" : "OFF");
}

void RelayController::setBuzzer(bool state) {
    digitalWrite(BUZZER_PIN, state ? HIGH : LOW);
    DEBUG_PRINTF("Buzzer set to %s\n", state ? "ON" : "OFF");
}

void RelayController::updateRelayState(uint8_t pin, bool state) {
    if (state) {
        current_relay_states |= (1 << pin);
    } else {
        current_relay_states &= ~(1 << pin);
    }
    
    // Write the complete state to PCF8574
    // Set feedback pins high to maintain input capability
    uint8_t output_value = current_relay_states;
    output_value |= (1 << FEEDBACK_COMPRESSOR_PIN);  // Keep feedback pins high
    output_value |= (1 << FEEDBACK_EVAP_FAN_PIN);
    
    pcf8574.write8(output_value);
    
    // Update global relay state
    SAFE_STATE_WRITE(status.relay_states, current_relay_states);
}

bool RelayController::getCompressorFeedback() {
    return readFeedbackPin(FEEDBACK_COMPRESSOR_PIN);
}

bool RelayController::getEvaporatorFanFeedback() {
    return readFeedbackPin(FEEDBACK_EVAP_FAN_PIN);
}

bool RelayController::readFeedbackPin(uint8_t pin) {
    // For PCF8574 quasi-bidirectional operation:
    // 1. Write HIGH to the pin to enable input mode
    // 2. Read the pin state
    // 3. External contact can pull pin LOW
    
    pcf8574.write(pin, HIGH);  // Enable input mode
    delay(1);  // Small delay for signal settling
    bool state = pcf8574.read(pin);
    
    // Note: For normally-open auxiliary contacts connected to ground,
    // a closed contact will pull the pin LOW, so we invert the reading
    return !state;  // Invert: LOW = contact closed = relay energized
}

void RelayController::setAllRelays(bool state) {
    if (state) {
        current_relay_states = 0x0F;  // Turn on all 4 relay pins
    } else {
        current_relay_states = 0x00;  // Turn off all relay pins
    }
    
    uint8_t output_value = current_relay_states;
    output_value |= (1 << FEEDBACK_COMPRESSOR_PIN);
    output_value |= (1 << FEEDBACK_EVAP_FAN_PIN);
    
    pcf8574.write8(output_value);
    
    // Update global state
    SAFE_STATE_WRITE(status.relay_states, current_relay_states);
    SAFE_STATE_WRITE(status.compressor_running, state);
    SAFE_STATE_WRITE(status.evap_fan_running, state);
}

void RelayController::emergencyStop() {
    DEBUG_PRINTLN("EMERGENCY STOP - All relays OFF");
    setAllRelays(false);
    setBuzzer(true);  // Sound alarm
    
    // Set system to fault state
    SAFE_STATE_WRITE(status.current_state, SystemState::FAULT_CONDITION);
}

uint8_t RelayController::getRelayStates() {
    return current_relay_states;
}

bool RelayController::verifyRelayOperation(uint8_t relay_pin, bool expected_state) {
    // For relays with feedback contacts, verify operation
    bool feedback_state = false;
    
    if (relay_pin == RELAY_COMPRESSOR_PIN) {
        feedback_state = getCompressorFeedback();
    } else if (relay_pin == RELAY_EVAP_FAN_PIN) {
        feedback_state = getEvaporatorFanFeedback();
    } else {
        // No feedback available for this relay
        return true;
    }
    
    return (feedback_state == expected_state);
}

bool RelayController::checkRelayFaults() {
    uint32_t current_time = millis();
    bool fault_detected = false;
    
    // Check compressor feedback
    bool compressor_commanded = (current_relay_states & (1 << RELAY_COMPRESSOR_PIN)) != 0;
    bool compressor_feedback = getCompressorFeedback();
    
    if (compressor_commanded != compressor_feedback) {
        if (!compressor_feedback_fault_active) {
            compressor_feedback_fault_start = current_time;
            compressor_feedback_fault_active = true;
        } else if (current_time - compressor_feedback_fault_start > FEEDBACK_FAULT_DELAY_MS) {
            // Persistent mismatch - this is a fault
            fault_detected = true;
            SAFE_STATE_WRITE(status.active_fault, FaultCode::COMPRESSOR_FEEDBACK_FAIL);
            DEBUG_PRINTLN("Compressor feedback fault detected");
        }
    } else {
        // Feedback matches command - reset fault
        compressor_feedback_fault_active = false;
    }
    
    // Check evaporator fan feedback
    bool fan_commanded = (current_relay_states & (1 << RELAY_EVAP_FAN_PIN)) != 0;
    bool fan_feedback = getEvaporatorFanFeedback();
    
    if (fan_commanded != fan_feedback) {
        if (!fan_feedback_fault_active) {
            fan_feedback_fault_start = current_time;
            fan_feedback_fault_active = true;
        } else if (current_time - fan_feedback_fault_start > FEEDBACK_FAULT_DELAY_MS) {
            // Persistent mismatch - this is a fault
            if (!fault_detected) {  // Only set if no higher priority fault
                fault_detected = true;
                SAFE_STATE_WRITE(status.active_fault, FaultCode::EVAP_FAN_FEEDBACK_FAIL);
                DEBUG_PRINTLN("Evaporator fan feedback fault detected");
            }
        }
    } else {
        // Feedback matches command - reset fault
        fan_feedback_fault_active = false;
    }
    
    // Check PCF8574 communication
    if (!isConnected()) {
        fault_detected = true;
        SAFE_STATE_WRITE(status.active_fault, FaultCode::PCF8574_COMM_FAIL);
        DEBUG_PRINTLN("PCF8574 communication fault detected");
        
        // Emergency stop on communication failure
        emergencyStop();
    }
    
    return fault_detected;
}

FaultCode RelayController::getRelayFaultCode() {
    if (!isConnected()) {
        return FaultCode::PCF8574_COMM_FAIL;
    }
    
    if (compressor_feedback_fault_active) {
        return FaultCode::COMPRESSOR_FEEDBACK_FAIL;
    }
    
    if (fan_feedback_fault_active) {
        return FaultCode::EVAP_FAN_FEEDBACK_FAIL;
    }
    
    return FaultCode::NONE;
}