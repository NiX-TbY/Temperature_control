#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config/feature_flags.h"

#ifdef ENABLE_RTC

// Simple PCF85063A RTC reader (time only, no alarms)
class RTCClock {
public:
    bool begin(TwoWire &bus = Wire);
    bool readTime(struct tm &out);
    String isoTimestamp();
    uint32_t lastReadMillis() const { return _lastRead; }
private:
    TwoWire *_wire = nullptr;
    bool _present = false;
    uint32_t _lastRead = 0;
    struct tm _cache {};
    bool readRaw();
    static uint8_t bcd2bin(uint8_t v) { return (v & 0x0F) + ((v >> 4) * 10); }
};

extern RTCClock rtcClock;

#endif // ENABLE_RTC
