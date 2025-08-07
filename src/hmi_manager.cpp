#include "hmi_manager.h"
#include <esp_heap_caps.h>

// Forward declaration
const char* getFaultMessage(FaultCode fault);

HMIManager& HMIManager::getInstance() {
    static HMIManager instance;
    return instance;
}

bool HMIManager::init() {
    DEBUG_PRINTLN("Initializing HMI Manager...");
    
    // Initialize member variables
    alarm_animation_active = false;
    defrost_animation_active = false;
    service_menu_visible = false;
    last_touch_time = 0;
    
    defrost_press_state = {false, 0, false};
    service_press_state = {false, 0, false};
    
    // Setup styles first
    setupStyles();
    
    // Create screens
    createMainScreen();
    createServiceMenu();
    
    // Setup animations
    setupAnimations();
    
    // Show main screen initially
    showMainScreen();
    
    DEBUG_PRINTLN("HMI Manager initialized successfully");
    return true;
}

void HMIManager::setupStyles() {
    // Main screen style
    lv_style_init(&style_main_screen);
    lv_style_set_bg_color(&style_main_screen, lv_color_black());
    lv_style_set_bg_opa(&style_main_screen, LV_OPA_COVER);
    
    // Temperature display style
    lv_style_init(&style_temp_display);
    lv_style_set_text_font(&style_temp_display, &lv_font_montserrat_48);
    lv_style_set_text_color(&style_temp_display, lv_color_white());
    lv_style_set_text_align(&style_temp_display, LV_TEXT_ALIGN_CENTER);
    
    // Temperature alarm style
    lv_style_init(&style_temp_alarm);
    lv_style_set_text_font(&style_temp_alarm, &lv_font_montserrat_48);
    lv_style_set_text_color(&style_temp_alarm, lv_color_make(255, 0, 0));
    lv_style_set_text_align(&style_temp_alarm, LV_TEXT_ALIGN_CENTER);
    
    // Normal button style
    lv_style_init(&style_button_normal);
    lv_style_set_bg_color(&style_button_normal, lv_color_make(0, 51, 102));
    lv_style_set_radius(&style_button_normal, 8);
    lv_style_set_border_width(&style_button_normal, 2);
    lv_style_set_border_color(&style_button_normal, lv_color_white());
    
    // Pressed button style
    lv_style_init(&style_button_pressed);
    lv_style_set_bg_color(&style_button_pressed, lv_color_make(0, 85, 153));
    
    // Defrost button style
    lv_style_init(&style_button_defrost);
    lv_style_set_bg_color(&style_button_defrost, lv_color_make(173, 142, 110));
    lv_style_set_radius(&style_button_defrost, 8);
    
    // Fault display style
    lv_style_init(&style_fault_display);
    lv_style_set_bg_color(&style_fault_display, lv_color_make(255, 0, 0));
    lv_style_set_text_color(&style_fault_display, lv_color_white());
    lv_style_set_text_font(&style_fault_display, &lv_font_montserrat_24);
    lv_style_set_text_align(&style_fault_display, LV_TEXT_ALIGN_CENTER);
    lv_style_set_radius(&style_fault_display, 4);
}

void HMIManager::createMainScreen() {
    screen_main = lv_obj_create(NULL);
    lv_obj_add_style(screen_main, &style_main_screen, 0);
    
    // Left column container
    lv_obj_t* left_col = lv_obj_create(screen_main);
    lv_obj_set_size(left_col, 211, 480);
    lv_obj_set_pos(left_col, 0, 0);
    lv_obj_set_style_bg_opa(left_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_col, 0, 0);
    lv_obj_set_flex_flow(left_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_col, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Setpoint display/fault display area
    label_setpoint = lv_label_create(left_col);
    lv_obj_set_size(label_setpoint, 190, 80);
    lv_obj_set_style_bg_color(label_setpoint, lv_color_make(22, 138, 239), 0);
    lv_obj_set_style_bg_opa(label_setpoint, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(label_setpoint, lv_color_black(), 0);
    lv_obj_set_style_text_font(label_setpoint, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_align(label_setpoint, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_setpoint, "-18°C");
    lv_obj_center(label_setpoint);
    
    // Fault display (overlays setpoint when active)
    label_fault_display = lv_label_create(left_col);
    lv_obj_set_size(label_fault_display, 190, 80);
    lv_obj_add_style(label_fault_display, &style_fault_display, 0);
    lv_label_set_long_mode(label_fault_display, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label_fault_display, "");
    lv_obj_add_flag(label_fault_display, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align_to(label_fault_display, label_setpoint, LV_ALIGN_CENTER, 0, 0);
    
    // Temperature up button
    btn_temp_up = lv_btn_create(left_col);
    lv_obj_set_size(btn_temp_up, 190, 80);
    lv_obj_add_style(btn_temp_up, &style_button_normal, 0);
    lv_obj_add_style(btn_temp_up, &style_button_pressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(btn_temp_up, tempUpButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* label_up = lv_label_create(btn_temp_up);
    lv_label_set_text(label_up, LV_SYMBOL_UP);
    lv_obj_set_style_text_font(label_up, &lv_font_montserrat_32, 0);
    lv_obj_center(label_up);
    
    // Temperature down button  
    btn_temp_down = lv_btn_create(left_col);
    lv_obj_set_size(btn_temp_down, 190, 80);
    lv_obj_add_style(btn_temp_down, &style_button_normal, 0);
    lv_obj_add_style(btn_temp_down, &style_button_pressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(btn_temp_down, tempDownButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* label_down = lv_label_create(btn_temp_down);
    lv_label_set_text(label_down, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_font(label_down, &lv_font_montserrat_32, 0);
    lv_obj_center(label_down);
    
    // Defrost button
    btn_defrost = lv_btn_create(left_col);
    lv_obj_set_size(btn_defrost, 190, 80);
    lv_obj_add_style(btn_defrost, &style_button_defrost, 0);
    lv_obj_add_event_cb(btn_defrost, defrostButtonCallback, LV_EVENT_ALL, this);
    lv_obj_t* label_defrost = lv_label_create(btn_defrost);
    lv_label_set_text(label_defrost, LV_SYMBOL_REFRESH " DEFROST");
    lv_obj_set_style_text_font(label_defrost, &lv_font_montserrat_20, 0);
    lv_obj_center(label_defrost);
    
    // Right column - main temperature display
    lv_obj_t* right_col = lv_obj_create(screen_main);
    lv_obj_set_size(right_col, 580, 480);
    lv_obj_set_pos(right_col, 220, 0);
    lv_obj_set_style_bg_opa(right_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_col, 0, 0);
    
    label_actual_temp = lv_label_create(right_col);
    lv_obj_add_style(label_actual_temp, &style_temp_display, 0);
    lv_label_set_text(label_actual_temp, "-18.2°C");
    lv_obj_center(label_actual_temp);
    
    // Hidden silence button
    btn_silence = lv_btn_create(screen_main);
    lv_obj_set_size(btn_silence, 180, 60);
    lv_obj_align(btn_silence, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(btn_silence, lv_color_black(), 0);
    lv_obj_set_style_border_color(btn_silence, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_border_width(btn_silence, 2, 0);
    lv_obj_add_event_cb(btn_silence, silenceButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_add_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t* label_silence = lv_label_create(btn_silence);
    lv_label_set_text(label_silence, "SILENCE");
    lv_obj_set_style_text_color(label_silence, lv_color_make(255, 0, 0), 0);
    lv_obj_center(label_silence);
    
    // Hidden service menu trigger area
    area_service_trigger = lv_obj_create(screen_main);
    lv_obj_set_size(area_service_trigger, 100, 100);
    lv_obj_align(area_service_trigger, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_opa(area_service_trigger, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(area_service_trigger, 0, 0);
    lv_obj_add_event_cb(area_service_trigger, serviceTriggerCallback, LV_EVENT_ALL, this);
}

void HMIManager::createServiceMenu() {
    screen_service = lv_obj_create(NULL);
    lv_obj_add_style(screen_service, &style_main_screen, 0);
    
    // Create tabview for organized service menu
    tabview_service = lv_tabview_create(screen_service, LV_DIR_TOP, 50);
    
    // Live Data tab
    tab_live_data = lv_tabview_add_tab(tabview_service, "Live Data");
    // ... Add live sensor readings display
    
    // Settings tab  
    tab_settings = lv_tabview_add_tab(tabview_service, "Settings");
    // ... Add adjustable parameters
    
    // Calibration tab
    tab_calibration = lv_tabview_add_tab(tabview_service, "Calibration");
    // ... Add sensor offset adjustments
    
    // Diagnostics tab
    tab_diagnostics = lv_tabview_add_tab(tabview_service, "Diagnostics");
    // ... Add fault history and system info
    
    // Close button
    lv_obj_t* btn_close = lv_btn_create(screen_service);
    lv_obj_set_size(btn_close, 100, 40);
    lv_obj_align(btn_close, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_event_cb(btn_close, serviceCloseCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* label_close = lv_label_create(btn_close);
    lv_label_set_text(label_close, "CLOSE");
    lv_obj_center(label_close);
}

void HMIManager::setupAnimations() {
    // Temperature pulse animation for alarms
    lv_anim_init(&anim_temp_pulse);
    lv_anim_set_var(&anim_temp_pulse, label_actual_temp);
    lv_anim_set_values(&anim_temp_pulse, 0, 255);
    lv_anim_set_exec_cb(&anim_temp_pulse, tempPulseAnimCallback);
    lv_anim_set_time(&anim_temp_pulse, 1000);
    lv_anim_set_playback_time(&anim_temp_pulse, 1000);
    lv_anim_set_repeat_count(&anim_temp_pulse, LV_ANIM_REPEAT_INFINITE);
    
    // Silence button fade-in animation
    lv_anim_init(&anim_silence_fade);
    lv_anim_set_var(&anim_silence_fade, btn_silence);
    lv_anim_set_values(&anim_silence_fade, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_exec_cb(&anim_silence_fade, opacityAnimCallback);
    lv_anim_set_time(&anim_silence_fade, 500);
    
    // Defrost button pulse animation
    lv_anim_init(&anim_defrost_pulse);
    lv_anim_set_var(&anim_defrost_pulse, btn_defrost);
    lv_anim_set_values(&anim_defrost_pulse, LV_OPA_COVER, LV_OPA_50);
    lv_anim_set_exec_cb(&anim_defrost_pulse, opacityAnimCallback);
    lv_anim_set_time(&anim_defrost_pulse, 800);
    lv_anim_set_playback_time(&anim_defrost_pulse, 800);
    lv_anim_set_repeat_count(&anim_defrost_pulse, LV_ANIM_REPEAT_INFINITE);
}

void HMIManager::update() {
    if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (g_state.hmi_needs_update) {
            // Update temperature displays
            updateTemperatureDisplay(g_state.sensors.cabin_temp_c, g_state.settings.setpoint_temp_c);
            
            // Update system status displays
            updateSystemStatus(g_state.status);
            
            // Handle animations based on state
            if (g_state.status.alarm_state == AlarmState::HIGH_TEMP_ACTIVE && !alarm_animation_active) {
                triggerAlarmAnimation(true);
            } else if (g_state.status.alarm_state == AlarmState::NONE && alarm_animation_active) {
                triggerAlarmAnimation(false);
            }
            
            if (g_state.status.defrost_active && !defrost_animation_active) {
                triggerDefrostAnimation(true);
            } else if (!g_state.status.defrost_active && defrost_animation_active) {
                triggerDefrostAnimation(false);
            }
            
            // Handle fault display
            if (g_state.status.active_fault != FaultCode::NONE) {
                const char* fault_msg = getFaultMessage(g_state.status.active_fault);
                updateFaultDisplay(fault_msg);
            } else {
                updateFaultDisplay("");
            }
            
            g_state.hmi_needs_update = false;
        }
        xSemaphoreGive(g_state.mutex);
    }
}

void HMIManager::updateTemperatureDisplay(float actual_temp, float setpoint_temp) {
    lv_label_set_text_fmt(label_actual_temp, "%.1f°C", actual_temp);
    lv_label_set_text_fmt(label_setpoint, "%.0f°C", setpoint_temp);
}

void HMIManager::updateSystemStatus(const SystemStatus& status) {
    // Update UI elements based on system status
    // This could include compressor indicators, fan status, etc.
}

void HMIManager::triggerAlarmAnimation(bool active) {
    if (active && !alarm_animation_active) {
        lv_anim_start(&anim_temp_pulse);
        lv_obj_clear_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
        lv_anim_start(&anim_silence_fade);
        alarm_animation_active = true;
    } else if (!active && alarm_animation_active) {
        lv_anim_del(label_actual_temp, tempPulseAnimCallback);
        lv_obj_remove_style(label_actual_temp, &style_temp_alarm, 0);
        lv_obj_add_style(label_actual_temp, &style_temp_display, 0);
        lv_obj_add_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
        alarm_animation_active = false;
    }
}

void HMIManager::triggerDefrostAnimation(bool active) {
    if (active && !defrost_animation_active) {
        lv_anim_start(&anim_defrost_pulse);
        defrost_animation_active = true;
    } else if (!active && defrost_animation_active) {
        lv_anim_del(btn_defrost, opacityAnimCallback);
        lv_obj_set_style_opa(btn_defrost, LV_OPA_COVER, 0);
        defrost_animation_active = false;
    }
}

void HMIManager::setAlarmAcknowledged(bool acknowledged) {
    if (acknowledged) {
        lv_anim_del(label_actual_temp, tempPulseAnimCallback);
        lv_obj_remove_style(label_actual_temp, &style_temp_display, 0);
        lv_obj_add_style(label_actual_temp, &style_temp_alarm, 0);
        lv_obj_add_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
    }
}

void HMIManager::updateFaultDisplay(const char* fault_text) {
    if (fault_text && strlen(fault_text) > 0) {
        lv_label_set_text(label_fault_display, fault_text);
        lv_obj_clear_flag(label_fault_display, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(label_setpoint, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(label_fault_display, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(label_setpoint, LV_OBJ_FLAG_HIDDEN);
    }
}

void HMIManager::showMainScreen() {
    lv_scr_load(screen_main);
    service_menu_visible = false;
}

void HMIManager::showServiceMenu() {
    lv_scr_load(screen_service);
    service_menu_visible = true;
    SAFE_STATE_WRITE(service_menu_active, true);
}

void HMIManager::hideServiceMenu() {
    showMainScreen();
    SAFE_STATE_WRITE(service_menu_active, false);
}

// Event callback implementations
void HMIManager::tempUpButtonCallback(lv_event_t* e) {
    HMIManager* hmi = static_cast<HMIManager*>(lv_event_get_user_data(e));
    // Increase setpoint by 1 degree
    if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_state.settings.setpoint_temp_c = constrain(g_state.settings.setpoint_temp_c + 1.0f, -30.0f, 0.0f);
        g_state.hmi_needs_update = true;
        xSemaphoreGive(g_state.mutex);
    }
}

void HMIManager::tempDownButtonCallback(lv_event_t* e) {
    HMIManager* hmi = static_cast<HMIManager*>(lv_event_get_user_data(e));
    // Decrease setpoint by 1 degree
    if (xSemaphoreTake(g_state.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_state.settings.setpoint_temp_c = constrain(g_state.settings.setpoint_temp_c - 1.0f, -30.0f, 0.0f);
        g_state.hmi_needs_update = true;
        xSemaphoreGive(g_state.mutex);
    }
}

void HMIManager::defrostButtonCallback(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    HMIManager* hmi = static_cast<HMIManager*>(lv_event_get_user_data(e));
    
    if (code == LV_EVENT_PRESSED) {
        hmi->defrost_press_state.is_pressed = true;
        hmi->defrost_press_state.press_start_time = lv_tick_get();
        hmi->defrost_press_state.action_triggered = false;
    } else if (code == LV_EVENT_PRESSING) {
        if (!hmi->defrost_press_state.action_triggered && 
            lv_tick_elaps(hmi->defrost_press_state.press_start_time) > MANUAL_DEFROST_HOLD_TIME_MS) {
            hmi->processDefrostLongPress();
            hmi->defrost_press_state.action_triggered = true;
        }
    } else if (code == LV_EVENT_RELEASED) {
        hmi->defrost_press_state.is_pressed = false;
    }
}

void HMIManager::silenceButtonCallback(lv_event_t* e) {
    // Silence alarm
    SAFE_STATE_WRITE(status.alarm_state, AlarmState::HIGH_TEMP_SILENCED);
    HMIManager::getInstance().setAlarmAcknowledged(true);
}

void HMIManager::serviceTriggerCallback(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    HMIManager* hmi = static_cast<HMIManager*>(lv_event_get_user_data(e));
    
    if (code == LV_EVENT_PRESSED) {
        hmi->service_press_state.is_pressed = true;
        hmi->service_press_state.press_start_time = lv_tick_get();
        hmi->service_press_state.action_triggered = false;
    } else if (code == LV_EVENT_PRESSING) {
        if (!hmi->service_press_state.action_triggered && 
            lv_tick_elaps(hmi->service_press_state.press_start_time) > SERVICE_MENU_HOLD_TIME_MS) {
            hmi->processServiceLongPress();
            hmi->service_press_state.action_triggered = true;
        }
    } else if (code == LV_EVENT_RELEASED) {
        hmi->service_press_state.is_pressed = false;
    }
}

void HMIManager::serviceCloseCallback(lv_event_t* e) {
    HMIManager::getInstance().hideServiceMenu();
}

void HMIManager::processDefrostLongPress() {
    DEBUG_PRINTLN("Manual defrost initiated!");
    SAFE_STATE_WRITE(status.manual_defrost_requested, true);
}

void HMIManager::processServiceLongPress() {
    DEBUG_PRINTLN("Service menu activated!");
    showServiceMenu();
}

// Animation callbacks
void HMIManager::tempPulseAnimCallback(void* var, int32_t v) {
    lv_obj_t* label = static_cast<lv_obj_t*>(var);
    lv_color_t color = lv_color_mix(lv_color_make(255, 0, 0), lv_color_white(), v);
    lv_obj_set_style_text_color(label, color, 0);
}

void HMIManager::opacityAnimCallback(void* var, int32_t v) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(var);
    lv_obj_set_style_opa(obj, v, 0);
}

const char* getFaultMessage(FaultCode fault) {
    switch (fault) {
        case FaultCode::CABIN_SENSOR_OPEN: return "CABIN SENSOR OPEN";
        case FaultCode::CABIN_SENSOR_SHORT: return "CABIN SENSOR SHORT";
        case FaultCode::EVAP_SENSOR_FAIL: return "EVAP SENSOR FAIL";
        case FaultCode::CONDENSER_SENSOR_FAIL: return "CONDENSER SENSOR FAIL";
        case FaultCode::SUCTION_SENSOR_FAIL: return "SUCTION SENSOR FAIL";
        case FaultCode::COMPRESSOR_FEEDBACK_FAIL: return "COMPRESSOR FEEDBACK FAIL";
        case FaultCode::EVAP_FAN_FEEDBACK_FAIL: return "FAN FEEDBACK FAIL";
        case FaultCode::PCF8574_COMM_FAIL: return "IO EXPANDER FAIL";
        case FaultCode::CH422G_COMM_FAIL: return "DISPLAY IO FAIL";
        case FaultCode::RTC_COMM_FAIL: return "RTC COMM FAIL";
        case FaultCode::TOUCH_COMM_FAIL: return "TOUCH COMM FAIL";
        default: return "";
    }
}