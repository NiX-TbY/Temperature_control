#include "ESP_IOExpander.h"

// CH422G Command/Register definitions based on reverse engineering
#define CH422G_REG_MODE     0x24
#define CH422G_REG_OUTPUT   0x38
#define CH422G_REG_INPUT    0x39

namespace esp_expander {

CH422G::CH422G(uint8_t scl_pin, uint8_t sda_pin, uint8_t address) 
    : _scl_pin(scl_pin), _sda_pin(sda_pin), _address(address), _pin_modes(0), _pin_states(0) {
}

CH422G::~CH422G() {
}

bool CH422G::init() {
    Wire.begin(_sda_pin, _scl_pin);
    return true;
}

bool CH422G::begin() {
    // Initialize CH422G mode register
    return writeRegister(CH422G_REG_MODE, 0x01); // Set to GPIO mode
}

void CH422G::pinMode(uint8_t pin, uint8_t mode) {
    if (pin > 7) return;
    
    if (mode == OUTPUT) {
        _pin_modes |= (1 << pin);
    } else {
        _pin_modes &= ~(1 << pin);
    }
    
    // Write mode configuration to device
    writeRegister(CH422G_REG_MODE, _pin_modes);
}

void CH422G::digitalWrite(uint8_t pin, uint8_t value) {
    if (pin > 7) return;
    
    if (value == HIGH) {
        _pin_states |= (1 << pin);
    } else {
        _pin_states &= ~(1 << pin);
    }
    
    writeRegister(CH422G_REG_OUTPUT, _pin_states);
}

uint8_t CH422G::digitalRead(uint8_t pin) {
    if (pin > 7) return LOW;
    
    // For quasi-bidirectional reading, first set pin HIGH
    digitalWrite(pin, HIGH);
    delay(1);
    
    uint8_t input_state = readRegister(CH422G_REG_INPUT);
    return (input_state & (1 << pin)) ? HIGH : LOW;
}

bool CH422G::isConnected() {
    Wire.beginTransmission(_address);
    return (Wire.endTransmission() == 0);
}

bool CH422G::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

uint8_t CH422G::readRegister(uint8_t reg) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.endTransmission();
    
    Wire.requestFrom(_address, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

} // namespace esp_expander