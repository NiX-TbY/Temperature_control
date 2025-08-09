#include "sensors/rtc_clock.h"
#include "config/config.h"

#ifdef ENABLE_RTC

RTCClock rtcClock;

static const uint8_t PCF85063_ADDR = 0x51; // PCF85063A 7-bit I2C address

bool RTCClock::begin(TwoWire &bus) {
    _wire = &bus;
    _wire->beginTransmission(PCF85063_ADDR);
    if (_wire->endTransmission() == 0) {
        _present = true;
        readRaw();
    } else {
        _present = false;
    }
    return _present;
}

bool RTCClock::readRaw() {
    if (!_present) return false;
    _wire->beginTransmission(PCF85063_ADDR);
    _wire->write((uint8_t)0x00); // seconds register
    if (_wire->endTransmission(false) != 0) return false;
    if (_wire->requestFrom(PCF85063_ADDR, (uint8_t)7) != 7) return false;
    uint8_t sec = _wire->read();
    uint8_t min = _wire->read();
    uint8_t hour = _wire->read();
    uint8_t day = _wire->read();
    _wire->read(); // weekday (ignore for now)
    uint8_t month = _wire->read();
    uint8_t year = _wire->read();
    memset(&_cache, 0, sizeof(_cache));
    _cache.tm_sec  = bcd2bin(sec & 0x7F);
    _cache.tm_min  = bcd2bin(min & 0x7F);
    _cache.tm_hour = bcd2bin(hour & 0x3F);
    _cache.tm_mday = bcd2bin(day & 0x3F);
    _cache.tm_mon  = bcd2bin(month & 0x1F) - 1; // 0-11
    _cache.tm_year = bcd2bin(year) + 100;       // years since 1900 (assume 20xx)
    _lastRead = millis();
    return true;
}

bool RTCClock::readTime(struct tm &out) {
    if (!_present) return false;
    if (millis() - _lastRead > 1000) {
        if (!readRaw()) return false;
    }
    out = _cache;
    return true;
}

String RTCClock::isoTimestamp() {
    if (!_present) return String("1970-01-01T00:00:00Z");
    struct tm t;
    if (!readTime(t)) return String("1970-01-01T00:00:00Z");
    char buf[25];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    return String(buf);
}

#endif // ENABLE_RTC
