#include "hal/ch422g.h"
#include "config/feature_flags.h"

CH422G ch422g; // Global instance

bool CH422G::probe() {
    _i2c->beginTransmission(CH422G_ADDRESS);
    return (_i2c->endTransmission() == 0);
}

bool CH422G::writeGPIO(uint8_t pinMask, uint8_t valueMask) {
    if (!_present) return false;
    // Update shadow register then push entire byte.
    _gpio_shadow &= ~pinMask;                 // clear targeted bits
    _gpio_shadow |= (valueMask & pinMask);    // set high bits
    uint8_t payload = _gpio_shadow;
    _i2c->beginTransmission(CH422G_ADDRESS);
    // TODO: If CH422G requires a register index, prepend it here.
    _i2c->write(payload);
    if (_i2c->endTransmission() != 0) {
        return false;
    }
    return true;
}

bool CH422G::digitalWrite(uint8_t pin, bool value) {
    if (!_present || pin > 7) return false;
    uint8_t mask = (1u << pin);
    return writeGPIO(mask, value ? mask : 0);
}

bool CH422G::setBacklight(uint8_t levelPercent) {
    if (!_present) return false;
    _backlight = constrain(levelPercent, 0u, 100u);
    bool on = (_backlight > 0);
    digitalWrite(CH422G_PIN_BACKLIGHT, on);
    DEBUG_PRINTF("CH422G setBacklight %u%% -> %s\n", _backlight, on ? "ON" : "OFF");
    return true;
}

bool CH422G::setDisplayEnable(bool on) {
    if (!_present) return false;
    DEBUG_PRINTF("CH422G setDisplayEnable %u\n", on ? 1 : 0);
    return digitalWrite(CH422G_PIN_DISP_EN, on);
}

bool CH422G::resetPanel() {
    if (!_present) return false;
    DEBUG_PRINTLN("CH422G resetPanel sequence");
    digitalWrite(CH422G_PIN_LCD_RST, false);
    delay(10);
    digitalWrite(CH422G_PIN_LCD_RST, true);
    delay(120);
    return true;
}

bool CH422G::resetTouch() {
    if (!_present) return false;
    DEBUG_PRINTLN("CH422G resetTouch sequence");
    digitalWrite(CH422G_PIN_CTP_RST, false);
    delay(5);
    digitalWrite(CH422G_PIN_CTP_RST, true);
    delay(50);
    return true;
}
