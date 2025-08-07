#pragma once

#include <Arduino.h>

class DisplayDriver {
public:
    DisplayDriver();
    bool init();
    void update();
    void clear();
    void setBacklight(bool enable);
    
private:
    bool _initialized;
};