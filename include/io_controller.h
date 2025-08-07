#pragma once
#include <PCF8574.h>

class RelayController {
public:
    RelayController(uint8_t address);
    bool begin();
    void setCompressorState(bool on);
    void setFanState(bool on);
    void setDefrostState(bool on);
    bool getCompressorFeedback();
    bool getFanFeedback();
    void setBuzzer(bool on);
    bool isConnected();

private:
    PCF8574 pcf;
    uint8_t current_relay_states;
};