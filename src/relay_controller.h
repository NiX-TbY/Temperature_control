#pragma once

#include <PCF8574.h>
#include "config.h"

class RelayController {
public:
    static RelayController& getInstance();
    
    bool init();
    bool isConnected();
    
    // Relay control
    void setCompressor(bool state);
    void setEvaporatorFan(bool state);
    void setDefrostHotGas(bool state);
    void setDefrostElectric(bool state);
    void setBuzzer(bool state);
    
    // Feedback reading
    bool getCompressorFeedback();
    bool getEvaporatorFanFeedback();
    
    // Batch operations
    void setAllRelays(bool state);
    void emergencyStop();
    
    // Status
    uint8_t getRelayStates();
    bool verifyRelayOperation(uint8_t relay_pin, bool expected_state);
    
    // Fault detection
    bool checkRelayFaults();
    FaultCode getRelayFaultCode();
    
private:
    RelayController() = default;
    ~RelayController() = default;
    RelayController(const RelayController&) = delete;
    RelayController& operator=(const RelayController&) = delete;
    
    PCF8574 pcf8574;
    uint8_t current_relay_states;
    
    // Fault tracking
    uint32_t compressor_feedback_fault_start;
    uint32_t fan_feedback_fault_start;
    bool compressor_feedback_fault_active;
    bool fan_feedback_fault_active;
    
    static constexpr uint32_t FEEDBACK_FAULT_DELAY_MS = 5000;  // 5 seconds
    
    // Internal methods
    void updateRelayState(uint8_t pin, bool state);
    bool readFeedbackPin(uint8_t pin);
    void checkFeedbackMismatch(uint8_t relay_pin, uint8_t feedback_pin, bool expected_state);
};