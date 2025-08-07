#pragma once

#include "config.h"
#include "sensor_manager.h"
#include "relay_controller.h"

class ControlLogic {
public:
    static ControlLogic& getInstance();
    
    bool init();
    void runControlLoop();
    
    // Manual controls
    void startManualDefrost();
    void stopDefrost();
    void acknowledgeAlarm();
    
    // State queries
    bool isDefrostActive() const;
    bool isAlarmActive() const;
    SystemState getCurrentState() const;
    
private:
    ControlLogic() = default;
    ~ControlLogic() = default;
    ControlLogic(const ControlLogic&) = delete;
    ControlLogic& operator=(const ControlLogic&) = delete;
    
    // Control state machine
    enum class ControlState {
        STARTUP,
        NORMAL_COOLING,
        COMPRESSOR_OFF_DELAY,
        DEFROST_ACTIVE,
        ALARM_CONDITION,
        FAULT_SAFE_MODE,
        BACKUP_COOLING_MODE
    };
    
    ControlState current_control_state;
    ControlState previous_control_state;
    uint32_t state_entry_time;
    
    // Timing variables
    uint32_t last_defrost_time;
    uint32_t compressor_on_time;
    uint32_t compressor_off_time;
    uint32_t alarm_trigger_time;
    uint32_t defrost_start_time;
    
    // Control parameters
    static constexpr uint32_t MIN_COMPRESSOR_ON_TIME_MS = 180000;   // 3 minutes
    static constexpr uint32_t MIN_COMPRESSOR_OFF_TIME_MS = 180000;  // 3 minutes
    static constexpr uint32_t STARTUP_DELAY_MS = 30000;            // 30 seconds
    static constexpr uint32_t MAX_DEFROST_TIME_MS = 1800000;       // 30 minutes
    static constexpr uint32_t DEFROST_INTERVAL_MS = 21600000;      // 6 hours
    static constexpr float BACKUP_COOLING_SETPOINT = -15.0f;       // Backup setpoint when cabin sensor fails
    
    // State machine methods
    void enterState(ControlState new_state);
    void handleStartupState();
    void handleNormalCoolingState();
    void handleCompressorOffDelayState();
    void handleDefrostActiveState();
    void handleAlarmConditionState();
    void handleFaultSafeModeState();
    void handleBackupCoolingModeState();
    
    // Control algorithms
    bool shouldStartCooling();
    bool shouldStopCooling();
    bool shouldStartDefrost();
    bool shouldStopDefrost();
    bool isHighTempAlarm();
    bool isLowTempAlarm();
    bool isCriticalFault();
    
    // Helper methods
    void startCooling();
    void stopCooling();
    void startDefrostCycle(DefrostType type);
    void runDefrostSequence();
    void updateAlarmState();
    void updateSystemTimers();
    void logStateChange();
    
    // Safety checks
    bool performSafetyChecks();
    void handleCriticalFault(FaultCode fault);
    void enterSafeMode();
};