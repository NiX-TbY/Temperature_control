#include "ESP_IOExpander.h"

ESP_IOExpander::ESP_IOExpander(uint8_t address) : _address(address), _pin_modes(0), _pin_states(0) {
}

ESP_IOExpander::~ESP_IOExpander() {
}

bool ESP_IOExpander::init() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    return true;
}

bool ESP_IOExpander::begin() {
    // Initialize CH422G with default configuration
    // Set system configuration - enable outputs
    bool success = writeRegister(CH422G_CMD_SYS_CFG, 0x37); // Enable all pins as outputs
    if (success) {
        // Clear all outputs initially
        success = writeRegister(CH422G_CMD_OUT_CTRL, 0x00);
    }
    return success;
}

void ESP_IOExpander::pinMode(uint8_t pin, uint8_t mode) {
    if (pin > 7) return;
    
    if (mode == OUTPUT) {
        _pin_modes |= (1 << pin);
    } else {
        _pin_modes &= ~(1 << pin);
    }
    
    // Update CH422G configuration
    writeRegister(CH422G_CMD_SYS_CFG, _pin_modes | 0x30);
}

void ESP_IOExpander::digitalWrite(uint8_t pin, uint8_t value) {
    if (pin > 7) return;
    
    if (value == HIGH) {
        _pin_states |= (1 << pin);
    } else {
        _pin_states &= ~(1 << pin);
    }
    
    // Update CH422G outputs
    writeRegister(CH422G_CMD_OUT_CTRL, _pin_states);
}

int ESP_IOExpander::digitalRead(uint8_t pin) {
    if (pin > 7) return LOW;
    
    // For input, first set pin high (quasi-bidirectional requirement)
    uint8_t temp_states = _pin_states | (1 << pin);
    writeRegister(CH422G_CMD_OUT_CTRL, temp_states);
    delay(1); // Small delay for signal to settle
    
    // Read input register
    uint8_t input_val = readRegister(CH422G_CMD_IN_READ);
    
    // Restore original output states
    writeRegister(CH422G_CMD_OUT_CTRL, _pin_states);
    
    return (input_val & (1 << pin)) ? HIGH : LOW;
}

void ESP_IOExpander::multiPinMode(uint32_t pin_mask, uint8_t mode) {
    for (int i = 0; i < 8; i++) {
        if (pin_mask & (1 << i)) {
            pinMode(i, mode);
        }
    }
}

void ESP_IOExpander::multiDigitalWrite(uint32_t pin_mask, uint32_t value) {
    for (int i = 0; i < 8; i++) {
        if (pin_mask & (1 << i)) {
            digitalWrite(i, (value & (1 << i)) ? HIGH : LOW);
        }
    }
}

bool ESP_IOExpander::isConnected() {
    Wire.beginTransmission(_address);
    return (Wire.endTransmission() == 0);
}

bool ESP_IOExpander::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

uint8_t ESP_IOExpander::readRegister(uint8_t reg) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return 0;
    }
    
    Wire.requestFrom(_address, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}