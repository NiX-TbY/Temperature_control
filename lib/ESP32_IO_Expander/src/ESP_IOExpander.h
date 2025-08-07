#pragma once

#include <Wire.h>
#include <Arduino.h>

class ESP_IOExpander {
public:
    ESP_IOExpander(uint8_t address = 0x71);
    ~ESP_IOExpander();
    
    bool init();
    bool begin();
    
    void pinMode(uint8_t pin, uint8_t mode);
    void digitalWrite(uint8_t pin, uint8_t value);
    int digitalRead(uint8_t pin);
    
    void multiPinMode(uint32_t pin_mask, uint8_t mode);
    void multiDigitalWrite(uint32_t pin_mask, uint32_t value);
    
    bool isConnected();
    
private:
    uint8_t _address;
    uint8_t _pin_modes;
    uint8_t _pin_states;
    
    bool writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    
    // CH422G specific command constants
    static constexpr uint8_t CH422G_CMD_SYS_CFG = 0x24;
    static constexpr uint8_t CH422G_CMD_OUT_CTRL = 0x38;
    static constexpr uint8_t CH422G_CMD_IN_READ = 0x26;
};