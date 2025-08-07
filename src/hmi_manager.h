#pragma once

#include <lvgl.h>
#include "config.h"

class HMIManager {
public:
    static HMIManager& getInstance();
    
    bool init();
    void update();
    void handleTouch(int16_t x, int16_t y, bool pressed);
    
    // UI state management
    void showMainScreen();
    void showServiceMenu();
    void hideServiceMenu();
    
    // Animation triggers
    void triggerAlarmAnimation(bool active);
    void triggerDefrostAnimation(bool active);
    void setAlarmAcknowledged(bool acknowledged);
    void updateFaultDisplay(const char* fault_text);
    
    // Data updates
    void updateTemperatureDisplay(float actual_temp, float setpoint_temp);
    void updateSystemStatus(const SystemStatus& status);
    
private:
    HMIManager() = default;
    ~HMIManager() = default;
    HMIManager(const HMIManager&) = delete;
    HMIManager& operator=(const HMIManager&) = delete;
    
    // LVGL objects
    lv_obj_t* screen_main;
    lv_obj_t* screen_service;
    
    // Main screen elements
    lv_obj_t* label_actual_temp;
    lv_obj_t* label_setpoint;
    lv_obj_t* label_fault_display;
    lv_obj_t* btn_temp_up;
    lv_obj_t* btn_temp_down;
    lv_obj_t* btn_defrost;
    lv_obj_t* btn_silence;
    lv_obj_t* area_service_trigger;
    
    // Service menu elements
    lv_obj_t* tabview_service;
    lv_obj_t* tab_live_data;
    lv_obj_t* tab_settings;
    lv_obj_t* tab_calibration;
    lv_obj_t* tab_diagnostics;
    
    // Animations
    lv_anim_t anim_temp_pulse;
    lv_anim_t anim_silence_fade;
    lv_anim_t anim_defrost_pulse;
    
    // Styles
    lv_style_t style_main_screen;
    lv_style_t style_temp_display;
    lv_style_t style_temp_alarm;
    lv_style_t style_button_normal;
    lv_style_t style_button_pressed;
    lv_style_t style_button_defrost;
    lv_style_t style_fault_display;
    
    // State tracking
    bool alarm_animation_active;
    bool defrost_animation_active;
    bool service_menu_visible;
    uint32_t last_touch_time;
    
    // Custom long press detection
    struct LongPressState {
        bool is_pressed;
        uint32_t press_start_time;
        bool action_triggered;
    };
    LongPressState defrost_press_state;
    LongPressState service_press_state;
    
    // Private methods
    void createMainScreen();
    void createServiceMenu();
    void setupStyles();
    void setupAnimations();
    
    // Event callbacks
    static void tempUpButtonCallback(lv_event_t* e);
    static void tempDownButtonCallback(lv_event_t* e);
    static void defrostButtonCallback(lv_event_t* e);
    static void silenceButtonCallback(lv_event_t* e);
    static void serviceTriggerCallback(lv_event_t* e);
    static void serviceCloseCallback(lv_event_t* e);
    
    // Animation callbacks
    static void tempPulseAnimCallback(void* var, int32_t v);
    static void opacityAnimCallback(void* var, int32_t v);
    
    // Long press handlers
    void handleLongPress(lv_obj_t* obj, LongPressState& state, uint32_t hold_time_ms);
    void processDefrostLongPress();
    void processServiceLongPress();
};