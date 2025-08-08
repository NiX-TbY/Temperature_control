#ifndef CH422G_H
#define CH422G_H

#include <Arduino.h>
#include <Wire.h>
#include "config/config.h"

// Minimal CH422G I/O Expander driver (expanded skeleton)
// Logical mapping of CH422G expander outputs; override via build flags if board differs.
#ifndef CH422G_PIN_DISP_EN
#define CH422G_PIN_DISP_EN 2
#endif
#ifndef CH422G_PIN_LCD_RST
#define CH422G_PIN_LCD_RST 3
#endif
#ifndef CH422G_PIN_CTP_RST
#define CH422G_PIN_CTP_RST 1
#endif
#ifndef CH422G_PIN_BACKLIGHT
#define CH422G_PIN_BACKLIGHT 0
#endif

class CH422G {
public:
    bool begin(TwoWire &i2c = Wire) {
        _i2c = &i2c;
        _present = probe();
        return _present;
    }

    bool isPresent() const { return _present; }

    // Panel / touch control (logical functions – depend on expander pin mapping)
    bool setDisplayEnable(bool on);
    bool resetPanel();
    bool resetTouch();

    // Backlight (0-100%) – placeholder implementation.
    bool setBacklight(uint8_t levelPercent);
    uint8_t getBacklight() const { return _backlight; }

    // Generic digital write (pin = logical expander line index 0..7)
    bool digitalWrite(uint8_t pin, bool value);

private:
    TwoWire *_i2c {nullptr};
    bool _present {false};
    uint8_t _backlight {100};

    bool probe();
    bool writeGPIO(uint8_t pinMask, uint8_t valueMask); // low-level write
    uint8_t _gpio_shadow {0};
};

extern CH422G ch422g;

#endif // CH422G_H
