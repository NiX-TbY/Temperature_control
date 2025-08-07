#include "control_logic.h"

ControlLogic& ControlLogic::getInstance() {
    static ControlLogic instance;
    return instance;
}

bool ControlLogic::init() {
    DEBUG_PRINTLN("Initializing Control Logic...");
    
    // Initialize state machine
    current_control_state = ControlState::STARTUP;
    previous_control_state = ControlState::STARTUP;
    state_entry_time = millis();
    
    // Initialize timing variables
    last_defrost_time = 0;
    compressor_on_time = 0;
    compressor_off_time = 0;
    alarm_trigger_time = 0;
    defrost_start_time = 0;
    
    // Initialize global state
    SAFE_STATE_WRITE(status.current_state, SystemState::STARTUP);
    SAFE_STATE_WRITE(status.alarm_state, AlarmState::NONE);
    SAFE_STATE_WRITE(status.active_fault, FaultCode::NONE);
    SAFE_STATE_WRITE(status.defrost_active, false);
    SAFE_STATE_WRITE(status.manual_defrost_requested, false);
    
    DEBUG_PRINTLN("Control Logic initialized successfully");
    return true;
}

void ControlLogic::runControlLoop() {
    uint32_t current_time = millis();
    
    // Perform safety checks first
    if (!performSafetyChecks()) {
        return;  // Critical fault detected, control loop aborted
    }
    
    // Check for manual defrost request
    bool manual_defrost = false;
    SAFE_STATE_READ(status.manual_defrost_requested, manual_defrost);
    if (manual_defrost && current_control_state != ControlState::DEFROST_ACTIVE) {
        startManualDefrost();
        SAFE_STATE_WRITE(status.manual_defrost_requested, false);
    }
    
    // Update system timers
    updateSystemTimers();
    
    // Run state machine
    switch (current_control_state) {
        case ControlState::STARTUP:
            handleStartupState();
            break;
        case ControlState::NORMAL_COOLING:
            handleNormalCoolingState();
            break;
        case ControlState::COMPRESSOR_OFF_DELAY:
            handleCompressorOffDelayState();
            break;
        case ControlState::DEFROST_ACTIVE:
            handleDefrostActiveState();
            break;
        case ControlState::ALARM_CONDITION:
            handleAlarmConditionState();
            break;
        case ControlState::FAULT_SAFE_MODE:
            handleFaultSafeModeState();
            break;
        case ControlState::BACKUP_COOLING_MODE:
            handleBackupCoolingModeState();
            break;
    }
    
    // Update alarm state
    updateAlarmState();
    
    // Update global system state
    SystemState sys_state;
    switch (current_control_state) {
        case ControlState::STARTUP:
            sys_state = SystemState::STARTUP;
            break;
        case ControlState::DEFROST_ACTIVE:
            sys_state = SystemState::DEFROST_CYCLE;
            break;
        case ControlState::ALARM_CONDITION:
            sys_state = SystemState::ALARM_ACTIVE;
            break;
        case ControlState::FAULT_SAFE_MODE:
        case ControlState::BACKUP_COOLING_MODE:
            sys_state = SystemState::FAULT_CONDITION;
            break;
        default:
            sys_state = SystemState::NORMAL_OPERATION;
            break;
    }
    SAFE_STATE_WRITE(status.current_state, sys_state);
}

void ControlLogic::enterState(ControlState new_state) {
    if (new_state != current_control_state) {
        previous_control_state = current_control_state;
        current_control_state = new_state;
        state_entry_time = millis();
        logStateChange();
    }
}

void ControlLogic::handleStartupState() {
    uint32_t elapsed_time = millis() - state_entry_time;
    
    if (elapsed_time > STARTUP_DELAY_MS) {
        // Check if we can proceed to normal operation
        if (SensorManager::getInstance().isSensorValid(SensorIndex::CABIN)) {
            enterState(ControlState::NORMAL_COOLING);
        } else {
            DEBUG_PRINTLN("Cabin sensor failed, entering backup cooling mode");
            enterState(ControlState::BACKUP_COOLING_MODE);
        }
    }
}

void ControlLogic::handleNormalCoolingState() {
    // Check for defrost requirement
    if (shouldStartDefrost()) {
        startDefrostCycle(DefrostType::HOT_GAS);
        return;
    }
    
    // Check for critical faults
    if (isCriticalFault()) {
        enterState(ControlState::FAULT_SAFE_MODE);
        return;
    }
    
    // Check cabin sensor status
    if (!SensorManager::getInstance().isSensorValid(SensorIndex::CABIN)) {
        DEBUG_PRINTLN("Cabin sensor failed during operation, switching to backup mode");
        enterState(ControlState::BACKUP_COOLING_MODE);
        return;
    }
    
    // Normal temperature control
    bool cooling_needed = shouldStartCooling();
    bool compressor_running = false;
    SAFE_STATE_READ(status.compressor_running, compressor_running);
    
    if (cooling_needed && !compressor_running) {
        // Check minimum off time
        if (compressor_off_time == 0 || (millis() - compressor_off_time) > MIN_COMPRESSOR_OFF_TIME_MS) {
            startCooling();
        }
    } else if (!cooling_needed && compressor_running) {
        // Check minimum on time
        if (compressor_on_time != 0 && (millis() - compressor_on_time) > MIN_COMPRESSOR_ON_TIME_MS) {
            stopCooling();
            enterState(ControlState::COMPRESSOR_OFF_DELAY);
        }
    }
}

void ControlLogic::handleCompressorOffDelayState() {
    uint32_t elapsed_time = millis() - state_entry_time;
    
    // Return to normal cooling after minimum off delay
    if (elapsed_time > MIN_COMPRESSOR_OFF_TIME_MS) {
        enterState(ControlState::NORMAL_COOLING);
    }
}

void ControlLogic::handleDefrostActiveState() {
    runDefrostSequence();
    
    if (shouldStopDefrost()) {
        stopDefrost();
        enterState(ControlState::NORMAL_COOLING);
    }
}

void ControlLogic::handleAlarmConditionState() {
    // Continue normal control but with alarm active
    // Alarm can be acknowledged or will auto-clear when condition resolves
    
    if (!isHighTempAlarm() && !isLowTempAlarm()) {
        // Alarm condition cleared
        SAFE_STATE_WRITE(status.alarm_state, AlarmState::NONE);
        enterState(ControlState::NORMAL_COOLING);
    }
}

void ControlLogic::handleFaultSafeModeState() {
    // Turn off all loads
    RelayController::getInstance().setAllRelays(false);
    
    // Check if fault is cleared
    FaultCode current_fault = FaultCode::NONE;
    SAFE_STATE_READ(status.active_fault, current_fault);
    
    if (current_fault == FaultCode::NONE) {
        DEBUG_PRINTLN("Fault cleared, returning to normal operation");
        enterState(ControlState::NORMAL_COOLING);
    }
}

void ControlLogic::handleBackupCoolingModeState() {
    // Use evaporator sensor or fixed timing for backup cooling
    bool evap_sensor_valid = SensorManager::getInstance().isSensorValid(SensorIndex::EVAPORATOR);
    
    if (evap_sensor_valid) {
        // Use evaporator temperature for control
        float evap_temp = SensorManager::getInstance().getTemperature(SensorIndex::EVAPORATOR);
        
        bool compressor_running = false;
        SAFE_STATE_READ(status.compressor_running, compressor_running);
        
        if (evap_temp > -5.0f && !compressor_running) {  // Start cooling
            startCooling();
        } else if (evap_temp < -15.0f && compressor_running) {  // Stop cooling
            stopCooling();
        }
    } else {
        // Use timed cooling cycles
        uint32_t cycle_time = (millis() - state_entry_time) % 1800000;  // 30-minute cycles
        
        bool compressor_running = false;
        SAFE_STATE_READ(status.compressor_running, compressor_running);
        
        if (cycle_time < 900000 && !compressor_running) {  // First 15 minutes - cooling
            startCooling();
        } else if (cycle_time >= 900000 && compressor_running) {  // Last 15 minutes - off
            stopCooling();
        }
    }
    
    // Check if cabin sensor is restored
    if (SensorManager::getInstance().isSensorValid(SensorIndex::CABIN)) {
        DEBUG_PRINTLN("Cabin sensor restored, returning to normal operation");
        enterState(ControlState::NORMAL_COOLING);
    }
}

bool ControlLogic::shouldStartCooling() {
    float cabin_temp, setpoint;
    SAFE_STATE_READ(sensors.cabin_temp_c, cabin_temp);
    SAFE_STATE_READ(settings.setpoint_temp_c, setpoint);
    SAFE_STATE_READ(settings.hysteresis_c, float hysteresis);
    
    return (cabin_temp > setpoint + hysteresis);
}

bool ControlLogic::shouldStopCooling() {
    float cabin_temp, setpoint;
    SAFE_STATE_READ(sensors.cabin_temp_c, cabin_temp);
    SAFE_STATE_READ(settings.setpoint_temp_c, setpoint);
    
    return (cabin_temp <= setpoint);
}

bool ControlLogic::shouldStartDefrost() {
    uint32_t current_time = millis();
    uint32_t time_since_last = current_time - last_defrost_time;
    
    // Automatic defrost based on time interval
    if (time_since_last > DEFROST_INTERVAL_MS) {
        return true;
    }
    
    // Demand defrost based on evaporator temperature (if sensor available)
    if (SensorManager::getInstance().isSensorValid(SensorIndex::EVAPORATOR)) {
        float evap_temp = SensorManager::getInstance().getTemperature(SensorIndex::EVAPORATOR);
        float cabin_temp = SensorManager::getInstance().getTemperature(SensorIndex::CABIN);
        
        // If evaporator is significantly colder than cabin, ice buildup likely
        if ((cabin_temp - evap_temp) > 15.0f) {
            return true;
        }
    }
    
    return false;
}

bool ControlLogic::shouldStopDefrost() {
    uint32_t defrost_duration = millis() - defrost_start_time;
    
    // Maximum time limit
    if (defrost_duration > MAX_DEFROST_TIME_MS) {
        return true;
    }
    
    // Temperature-based termination
    if (SensorManager::getInstance().isSensorValid(SensorIndex::EVAPORATOR)) {
        float evap_temp = SensorManager::getInstance().getTemperature(SensorIndex::EVAPORATOR);
        float termination_temp;
        SAFE_STATE_READ(settings.defrost_termination_temp_c, termination_temp);
        
        if (evap_temp >= termination_temp) {
            return true;
        }
    }
    
    return false;
}

bool ControlLogic::isHighTempAlarm() {
    float cabin_temp, setpoint, alarm_diff;
    SAFE_STATE_READ(sensors.cabin_temp_c, cabin_temp);
    SAFE_STATE_READ(settings.setpoint_temp_c, setpoint);
    SAFE_STATE_READ(settings.alarm_differential_c, alarm_diff);
    
    return (cabin_temp > setpoint + alarm_diff);
}

bool ControlLogic::isLowTempAlarm() {
    float cabin_temp, setpoint;
    SAFE_STATE_READ(sensors.cabin_temp_c, cabin_temp);
    SAFE_STATE_READ(settings.setpoint_temp_c, setpoint);
    
    // Low temp alarm at 5°C below setpoint
    return (cabin_temp < setpoint - 5.0f);
}

bool ControlLogic::isCriticalFault() {
    FaultCode fault;
    SAFE_STATE_READ(status.active_fault, fault);
    
    // Critical faults that require immediate shutdown
    return (fault == FaultCode::PCF8574_COMM_FAIL ||
            fault == FaultCode::COMPRESSOR_FEEDBACK_FAIL);
}

void ControlLogic::startCooling() {
    RelayController::getInstance().setCompressor(true);
    RelayController::getInstance().setEvaporatorFan(true);
    compressor_on_time = millis();
    DEBUG_PRINTLN("Cooling started");
}

void ControlLogic::stopCooling() {
    RelayController::getInstance().setCompressor(false);
    RelayController::getInstance().setEvaporatorFan(false);
    compressor_off_time = millis();
    DEBUG_PRINTLN("Cooling stopped");
}

void ControlLogic::startDefrostCycle(DefrostType type) {
    DEBUG_PRINTF("Starting %s defrost cycle\n", (type == DefrostType::HOT_GAS) ? "hot gas" : "electric");
    
    // Stop cooling first
    stopCooling();
    
    // Wait briefly for system to settle
    delay(5000);
    
    // Start defrost
    if (type == DefrostType::HOT_GAS) {
        RelayController::getInstance().setDefrostHotGas(true);
    } else {
        RelayController::getInstance().setDefrostElectric(true);
    }
    
    defrost_start_time = millis();
    last_defrost_time = defrost_start_time;
    
    SAFE_STATE_WRITE(status.defrost_active, true);
    enterState(ControlState::DEFROST_ACTIVE);
}

void ControlLogic::runDefrostSequence() {
    // Defrost sequence is primarily time and temperature based
    // The shouldStopDefrost() method handles the termination logic
    
    // Keep evaporator fan running during defrost to circulate air
    RelayController::getInstance().setEvaporatorFan(true);
}

void ControlLogic::stopDefrost() {
    DEBUG_PRINTLN("Stopping defrost cycle");
    
    RelayController::getInstance().setDefrostHotGas(false);
    RelayController::getInstance().setDefrostElectric(false);
    
    SAFE_STATE_WRITE(status.defrost_active, false);
    
    // Brief delay before resuming normal cooling
    delay(30000);  // 30 seconds
}

void ControlLogic::startManualDefrost() {
    if (current_control_state != ControlState::DEFROST_ACTIVE) {
        DEBUG_PRINTLN("Manual defrost initiated");
        DefrostType type;
        SAFE_STATE_READ(settings.defrost_type, type);
        startDefrostCycle(type);
    }
}

void ControlLogic::updateAlarmState() {
    AlarmState current_alarm;
    SAFE_STATE_READ(status.alarm_state, current_alarm);
    
    if (isHighTempAlarm()) {
        if (current_alarm == AlarmState::NONE) {
            SAFE_STATE_WRITE(status.alarm_state, AlarmState::HIGH_TEMP_ACTIVE);
            RelayController::getInstance().setBuzzer(true);
            enterState(ControlState::ALARM_CONDITION);
        }
    } else if (isLowTempAlarm()) {
        if (current_alarm == AlarmState::NONE) {
            SAFE_STATE_WRITE(status.alarm_state, AlarmState::LOW_TEMP_ACTIVE);
            RelayController::getInstance().setBuzzer(true);
            enterState(ControlState::ALARM_CONDITION);
        }
    }
}

void ControlLogic::acknowledgeAlarm() {
    AlarmState current_alarm;
    SAFE_STATE_READ(status.alarm_state, current_alarm);
    
    if (current_alarm == AlarmState::HIGH_TEMP_ACTIVE) {
        SAFE_STATE_WRITE(status.alarm_state, AlarmState::HIGH_TEMP_SILENCED);
        RelayController::getInstance().setBuzzer(false);
        DEBUG_PRINTLN("High temperature alarm silenced");
    } else if (current_alarm == AlarmState::LOW_TEMP_ACTIVE) {
        SAFE_STATE_WRITE(status.alarm_state, AlarmState::LOW_TEMP_SILENCED);
        RelayController::getInstance().setBuzzer(false);
        DEBUG_PRINTLN("Low temperature alarm silenced");
    }
}

bool ControlLogic::performSafetyChecks() {
    // Check sensor faults
    SensorManager::getInstance().checkSensorFaults();
    
    // Check relay faults
    RelayController::getInstance().checkRelayFaults();
    
    // Check for critical faults
    if (isCriticalFault()) {
        FaultCode fault;
        SAFE_STATE_READ(status.active_fault, fault);
        handleCriticalFault(fault);
        return false;
    }
    
    return true;
}

void ControlLogic::handleCriticalFault(FaultCode fault) {
    DEBUG_PRINTF("Critical fault detected: %d\n", static_cast<uint8_t>(fault));
    enterSafeMode();
}

void ControlLogic::enterSafeMode() {
    enterState(ControlState::FAULT_SAFE_MODE);
    RelayController::getInstance().emergencyStop();
}

void ControlLogic::updateSystemTimers() {
    // Update runtime counters, etc.
    // This can be used for maintenance scheduling
}

void ControlLogic::logStateChange() {
    DEBUG_PRINTF("Control state changed: %d -> %d\n", 
                 static_cast<int>(previous_control_state), 
                 static_cast<int>(current_control_state));
}

bool ControlLogic::isDefrostActive() const {
    return (current_control_state == ControlState::DEFROST_ACTIVE);
}

bool ControlLogic::isAlarmActive() const {
    return (current_control_state == ControlState::ALARM_CONDITION);
}

SystemState ControlLogic::getCurrentState() const {
    SystemState state;
    SAFE_STATE_READ(status.current_state, state);
    return state;
}