#pragma once
#include <PCF8574.h>
#include "config.h"

class RelayController {
public:
    RelayController(uint8_t address);
    bool begin();
    
    void setCompressorState(bool on);
    void setFanState(bool on);
    void setDefrostHotGasState(bool on);
    void setDefrostElectricState(bool on);
    
    bool getCompressorFeedback();
    bool getFanFeedback();
    
    void setBuzzer(bool on);
    
    bool isConnected();
    void updateRelayStates();

private:
    PCF8574 pcf;
    uint8_t current_relay_states;
    SemaphoreHandle_t i2c_mutex;
    
    void writeRelayState(uint8_t pin, bool state);
    bool readFeedback(uint8_t pin);
};

extern RelayController* g_relay_controller;