// Minimal GT911 capacitive touch controller driver (5-point) for Waveshare ESP32-S3 4.3" panel
// Provides init, readTouch querying first touch point. Expansion ready for multi-point.

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "config/config.h"

class GT911 {
public:
    bool begin(TwoWire &bus = Wire, uint8_t addr1 = GT911_ADDRESS_1, uint8_t addr2 = GT911_ADDRESS_2) {
        _wire = &bus;
        // Try primary then secondary
        _address = addr1;
        if (!probe()) {
            _address = addr2;
            if (!probe()) {
                _present = false;
                return false;
            }
        }
        _present = true;
        return true;
    }

    bool isPresent() const { return _present; }

    bool readTouch(uint16_t &x, uint16_t &y, bool &pressed) {
        if (!_present) { pressed = false; return false; }
        // GT911 data register 0x814E: status, then points starting at 0x8150
        if (!writeReg16(0x814E)) { pressed = false; return false; }
        uint8_t buf[8];
        if (_wire->requestFrom((int)_address, 8) != 8) { pressed = false; return false; }
        uint8_t status = buf[0] = _wire->read();
        uint8_t points = status & 0x0F;
        for (int i=1;i<8;i++) buf[i] = _wire->read();
        if (points == 0) { pressed = false; clearStatus(); return true; }
        // First point data at 0x8150 (after status read we already have first 7 bytes starting at 0x814E)
        uint16_t tx = (uint16_t)buf[3] << 8 | buf[2];
        uint16_t ty = (uint16_t)buf[5] << 8 | buf[4];
        if (tx >= DISPLAY_WIDTH || ty >= DISPLAY_HEIGHT) { pressed = false; clearStatus(); return false; }
        x = tx; y = ty; pressed = true; clearStatus();
        return true;
    }

private:
    TwoWire *_wire {nullptr};
    uint8_t _address {0};
    bool _present {false};

    bool probe() {
        _wire->beginTransmission(_address);
        return _wire->endTransmission() == 0; }

    bool writeReg16(uint16_t reg) {
        _wire->beginTransmission(_address);
        _wire->write(reg >> 8); _wire->write(reg & 0xFF);
        return _wire->endTransmission(false) == 0; }

    void clearStatus() {
        _wire->beginTransmission(_address);
        _wire->write(0x81); _wire->write(0x4E); _wire->write(0x00);
        _wire->endTransmission();
    }
};

extern GT911 gt911;
