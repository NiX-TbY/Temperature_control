#pragma once

#include <Arduino.h>
#include <Wire.h>

namespace esp_expander {

class CH422G {
public:
    CH422G(uint8_t scl_pin, uint8_t sda_pin, uint8_t address);
    ~CH422G();
    
    bool init();
    bool begin();
    
    void pinMode(uint8_t pin, uint8_t mode);
    void digitalWrite(uint8_t pin, uint8_t value);
    uint8_t digitalRead(uint8_t pin);
    
    bool isConnected();

private:
    uint8_t _scl_pin;
    uint8_t _sda_pin;
    uint8_t _address;
    uint8_t _pin_modes;
    uint8_t _pin_states;
    
    bool writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
};

} // namespace esp_expander