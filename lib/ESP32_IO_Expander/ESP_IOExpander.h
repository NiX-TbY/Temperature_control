#pragma once
#include <Arduino.h>
#include <Wire.h>

// Simplified ESP IO Expander library interface for compilation
namespace esp_expander {
    class CH422G {
    private:
        int _scl_pin;
        int _sda_pin;
        uint8_t _address;
        bool _initialized;

    public:
        CH422G(int scl_pin, int sda_pin, uint8_t address) : 
            _scl_pin(scl_pin), _sda_pin(sda_pin), _address(address), _initialized(false) {
            Serial.println("CH422G constructor called");
        }

        bool init() {
            Serial.println("CH422G init()");
            Wire.begin(_sda_pin, _scl_pin);
            _initialized = true;
            return true;
        }

        bool begin() {
            Serial.println("CH422G begin()");
            if (!_initialized) {
                return init();
            }
            
            // Test communication with CH422G
            Wire.beginTransmission(_address);
            uint8_t error = Wire.endTransmission();
            
            if (error == 0) {
                Serial.println("CH422G communication established");
                return true;
            } else {
                Serial.printf("CH422G communication failed with error: %d\n", error);
                return false;
            }
        }

        void pinMode(uint8_t pin, uint8_t mode) {
            Serial.printf("CH422G pinMode pin:%d mode:%d\n", pin, mode);
            // In real implementation, this would configure the CH422G pin mode
            // For compilation purposes, this is a placeholder
        }

        void digitalWrite(uint8_t pin, uint8_t value) {
            Serial.printf("CH422G digitalWrite pin:%d value:%d\n", pin, value);
            
            // Simplified CH422G write command
            // Real implementation would use proper CH422G command protocol
            Wire.beginTransmission(_address);
            Wire.write(0x38); // Example command register for output
            Wire.write(value ? (1 << pin) : 0);
            Wire.endTransmission();
        }

        uint8_t digitalRead(uint8_t pin) {
            Serial.printf("CH422G digitalRead pin:%d\n", pin);
            
            // Simplified read operation
            // Real implementation would use proper CH422G command protocol
            Wire.requestFrom(_address, (uint8_t)1);
            if (Wire.available()) {
                uint8_t data = Wire.read();
                return (data & (1 << pin)) ? HIGH : LOW;
            }
            return LOW;
        }

        bool isConnected() {
            Wire.beginTransmission(_address);
            return (Wire.endTransmission() == 0);
        }
    };
}