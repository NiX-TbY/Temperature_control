#ifndef RELAY_CONTROLLER_H
#define RELAY_CONTROLLER_H

#include <Arduino.h>
#include <Wire.h>
#include "config/config.h"

// PCF8574 8-bit I/O expander – used to drive refrigeration relays
// Lines (example mapping – adjust to actual board wiring):
// P0: COMPRESSOR
// P1: HOTGAS (defrost / reverse valve)
// P2: ELECTRIC_HEATER
// P3: FAN_MAIN
// P4: FAN_AUX (optional) / SPARE
// P5: ALARM (external) / SPARE
// P6: SPARE
// P7: SPARE

#ifndef PCF8574_ADDRESS
#define PCF8574_ADDRESS 0x20
#endif

enum RelayLine : uint8_t {
    RELAY_COMPRESSOR = 0,
    RELAY_HOTGAS     = 1,
    RELAY_ELECTRIC_HEATER = 2,
    RELAY_FAN_MAIN   = 3,
    RELAY_FAN_AUX    = 4,
    RELAY_ALARM      = 5,
};

class RelayController {
public:
    RelayController() : _bus(nullptr), _present(false), _shadow(0xFF) {}

    bool begin(TwoWire &bus = Wire) {
        _bus = &bus;
        _bus->beginTransmission(PCF8574_ADDRESS);
        if (_bus->endTransmission() == 0) {
            _present = true;
            _shadow = 0xFF; // all HIGH (inactive)
            writeShadow();
            DEBUG_PRINTLN("PCF8574 relay expander detected");
        } else {
            DEBUG_PRINTLN("WARNING: PCF8574 relay expander not detected");
            _present = false;
        }
        return _present;
    }

    bool isPresent() const { return _present; }

    void setRelay(RelayLine line, bool on) {
        if (!_present) return;
        uint8_t bit = static_cast<uint8_t>(line);
        if (on) {
            _shadow &= ~(1 << bit); // active LOW assumption
        } else {
            _shadow |= (1 << bit);
        }
        writeShadow();
    }

    bool getRelay(RelayLine line) const {
        uint8_t bit = static_cast<uint8_t>(line);
        return (_shadow & (1 << bit)) == 0; // LOW = ON
    }

    void allOff() {
        if (!_present) return;
        _shadow = 0xFF;
        writeShadow();
    }

    uint8_t rawState() const { return _shadow; }

private:
    TwoWire *_bus;
    bool _present;
    uint8_t _shadow; // Active LOW bitmap

    void writeShadow() {
        if (!_bus) return;
        _bus->beginTransmission(PCF8574_ADDRESS);
        _bus->write(_shadow);
        _bus->endTransmission();
    }
};

extern RelayController relays;

#endif // RELAY_CONTROLLER_H
