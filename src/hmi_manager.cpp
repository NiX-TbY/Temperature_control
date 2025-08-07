#include "hmi_manager.h"
#include "config.h"

// LVGL Objects
static lv_obj_t * scr_main;
static lv_obj_t * label_set_temp;
static lv_obj_t * label_actual_temp;
static lv_obj_t * btn_defrost;
static lv_obj_t * btn_silence;
static lv_obj_t * scr_service_menu;
static lv_obj_t * label_fault_display;

// LVGL Animations
static lv_anim_t anim_temp_pulse_color;
static lv_anim_t anim_temp_pulse_opa;
static lv_anim_t anim_silence_fade_in;
static lv_anim_t anim_defrost_pulse;

// LVGL Styles
static lv_style_t style_btn_blue;
static lv_style_t style_btn_blue_pressed;
static lv_style_t style_btn_lightblue;
static lv_style_t style_actual_temp_normal;
static lv_style_t style_actual_temp_alarm;

// Animation callbacks
static void temp_color_anim_cb(void * var, int32_t v) {
    lv_obj_set_style_text_color((lv_obj_t*)var, lv_color_mix(lv_color_hex(0xFF0000), lv_color_white(), v), 0);
}

static void opacity_anim_cb(void * var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)var, v, 0);
}

// Event Handlers
static void silence_btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
        g_system_state.alarm_status = AlarmState::HIGH_TEMP_SILENCED;
        xSemaphoreGive(g_system_state.mutex);
        
        hmi_set_alarm_acknowledged(true);
        // Logic to start 20-min silence timer will be in control_task
    }
}

static void long_press_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    
    // Use user_data to store press state: 0=released, 1=pressed, >1=start_tick
    uint32_t* press_state = (uint32_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_PRESSED) {
        *press_state = lv_tick_get();
    } else if (code == LV_EVENT_PRESSING) {
        if (*press_state > 1) { // Check if not already triggered
            uint32_t hold_duration_ms = (target == btn_defrost)? MANUAL_DEFROST_HOLD_MS : SERVICE_MENU_HOLD_MS;
            if (lv_tick_elaps(*press_state) > hold_duration_ms) {
                if (target == btn_defrost) {
                    // Signal control task to start manual defrost
                    xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
                    g_system_state.defrost_active = true;
                    xSemaphoreGive(g_system_state.mutex);
                    Serial.println("Manual defrost triggered!");
                } else { // Service Menu hidden area
                    lv_obj_clear_flag(scr_service_menu, LV_OBJ_FLAG_HIDDEN);
                    Serial.println("Service menu opened!");
                }
                *press_state = 1; // Mark as triggered to prevent re-triggering
            }
        }
    } else if (code == LV_EVENT_RELEASED) {
        *press_state = 0; // Reset state
    }
}

void hmi_init() {
    scr_main = lv_scr_act();
    lv_obj_set_style_bg_color(scr_main, lv_color_black(), 0);

    // --- Create Styles ---
    lv_style_init(&style_btn_blue);
    lv_style_set_bg_color(&style_btn_blue, lv_color_hex(0x003366));
    lv_style_set_radius(&style_btn_blue, 8);

    lv_style_init(&style_btn_blue_pressed);
    lv_style_set_bg_color(&style_btn_blue_pressed, lv_color_hex(0x005599));
    
    lv_style_init(&style_btn_lightblue);
    lv_style_set_bg_color(&style_btn_lightblue, lv_color_hex(0xAD8E6E));
    lv_style_set_radius(&style_btn_lightblue, 8);

    lv_style_init(&style_actual_temp_normal);
    lv_style_set_text_font(&style_actual_temp_normal, &lv_font_montserrat_48);
    lv_style_set_text_color(&style_actual_temp_normal, lv_color_white());

    lv_style_init(&style_actual_temp_alarm);
    lv_style_set_text_font(&style_actual_temp_alarm, &lv_font_montserrat_48);
    lv_style_set_text_color(&style_actual_temp_alarm, lv_color_hex(0xFF0000));

    // --- Left Column ---
    lv_obj_t* left_col = lv_obj_create(scr_main);
    lv_obj_set_size(left_col, 211, 480);
    lv_obj_set_pos(left_col, 0, 0);
    lv_obj_set_style_bg_opa(left_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_col, 0, 0);
    lv_obj_set_flex_flow(left_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_col, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    label_set_temp = lv_label_create(left_col);
    lv_obj_set_size(label_set_temp, 200, 100);
    lv_obj_set_style_bg_color(label_set_temp, lv_color_hex(0x168AEEF), 0);
    lv_obj_set_style_bg_opa(label_set_temp, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(label_set_temp, lv_color_black(), 0);
    lv_obj_set_style_text_font(label_set_temp, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_align(label_set_temp, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_set_temp, "-18");

    // Fault display label (overlaps set temp when active)
    label_fault_display = lv_label_create(left_col);
    lv_obj_set_size(label_fault_display, 200, 100);
    lv_obj_set_style_bg_color(label_fault_display, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(label_fault_display, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(label_fault_display, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_fault_display, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(label_fault_display, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(label_fault_display, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label_fault_display, "");
    lv_obj_add_flag(label_fault_display, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align_to(label_fault_display, label_set_temp, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* btn_up = lv_btn_create(left_col);
    lv_obj_set_size(btn_up, 200, 100);
    lv_obj_add_style(btn_up, &style_btn_blue, 0);
    lv_obj_add_style(btn_up, &style_btn_blue_pressed, LV_STATE_PRESSED);
    lv_obj_t* label_up = lv_label_create(btn_up);
    lv_label_set_text(label_up, LV_SYMBOL_UP);
    lv_obj_center(label_up);

    lv_obj_t* btn_down = lv_btn_create(left_col);
    lv_obj_set_size(btn_down, 200, 100);
    lv_obj_add_style(btn_down, &style_btn_blue, 0);
    lv_obj_add_style(btn_down, &style_btn_blue_pressed, LV_STATE_PRESSED);
    lv_obj_t* label_down = lv_label_create(btn_down);
    lv_label_set_text(label_down, LV_SYMBOL_DOWN);
    lv_obj_center(label_down);

    btn_defrost = lv_btn_create(left_col);
    lv_obj_set_size(btn_defrost, 200, 100);
    lv_obj_add_style(btn_defrost, &style_btn_lightblue, 0);
    lv_obj_t* label_defrost = lv_label_create(btn_defrost);
    lv_label_set_text(label_defrost, "DEFROST");
    lv_obj_center(label_defrost);
    static uint32_t defrost_press_state = 0;
    lv_obj_add_event_cb(btn_defrost, long_press_event_cb, LV_EVENT_ALL, &defrost_press_state);

    // --- Right Column ---
    lv_obj_t* right_col = lv_obj_create(scr_main);
    lv_obj_set_size(right_col, 580, 480);
    lv_obj_set_pos(right_col, 220, 0);
    lv_obj_set_style_bg_opa(right_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_col, 0, 0);
    
    label_actual_temp = lv_label_create(right_col);
    lv_obj_add_style(label_actual_temp, &style_actual_temp_normal, 0);
    lv_obj_set_style_text_align(label_actual_temp, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_actual_temp, "-18.2");
    lv_obj_center(label_actual_temp);

    // --- Hidden Elements ---
    btn_silence = lv_btn_create(scr_main);
    lv_obj_set_size(btn_silence, 200, 80);
    lv_obj_align(btn_silence, LV_ALIGN_BOTTOM_RIGHT, -50, -20);
    lv_obj_set_style_bg_color(btn_silence, lv_color_black(), 0);
    lv_obj_set_style_border_color(btn_silence, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(btn_silence, 2, 0);
    lv_obj_add_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(btn_silence, silence_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* label_silence = lv_label_create(btn_silence);
    lv_label_set_text(label_silence, "SILENCE");
    lv_obj_set_style_text_color(label_silence, lv_color_hex(0xFF0000), 0);
    lv_obj_center(label_silence);

    lv_obj_t* service_menu_area = lv_obj_create(scr_main);
    lv_obj_set_size(service_menu_area, 100, 100);
    lv_obj_align(service_menu_area, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_opa(service_menu_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(service_menu_area, 0, 0);
    static uint32_t service_press_state = 0;
    lv_obj_add_event_cb(service_menu_area, long_press_event_cb, LV_EVENT_ALL, &service_press_state);

    scr_service_menu = lv_obj_create(NULL); // Create screen but don't load it
    lv_obj_set_style_bg_color(scr_service_menu, lv_color_black(), 0);
    lv_obj_t* service_label = lv_label_create(scr_service_menu);
    lv_label_set_text(service_label, "SERVICE MENU\n(Implementation Pending)");
    lv_obj_set_style_text_color(service_label, lv_color_white(), 0);
    lv_obj_center(service_label);
    lv_obj_add_flag(scr_service_menu, LV_OBJ_FLAG_HIDDEN);

    // --- Initialize Animations ---
    lv_anim_init(&anim_temp_pulse_color);
    lv_anim_set_var(&anim_temp_pulse_color, label_actual_temp);
    lv_anim_set_values(&anim_temp_pulse_color, 0, 255);
    lv_anim_set_exec_cb(&anim_temp_pulse_color, temp_color_anim_cb);
    lv_anim_set_time(&anim_temp_pulse_color, 1000);
    lv_anim_set_playback_time(&anim_temp_pulse_color, 1000);
    lv_anim_set_repeat_count(&anim_temp_pulse_color, LV_ANIM_REPEAT_INFINITE);

    lv_anim_init(&anim_silence_fade_in);
    lv_anim_set_var(&anim_silence_fade_in, btn_silence);
    lv_anim_set_values(&anim_silence_fade_in, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_exec_cb(&anim_silence_fade_in, opacity_anim_cb);
    lv_anim_set_time(&anim_silence_fade_in, 500);

    lv_anim_init(&anim_defrost_pulse);
    lv_anim_set_var(&anim_defrost_pulse, btn_defrost);
    lv_anim_set_values(&anim_defrost_pulse, LV_OPA_COVER, LV_OPA_50);
    lv_anim_set_exec_cb(&anim_defrost_pulse, opacity_anim_cb);
    lv_anim_set_time(&anim_defrost_pulse, 800);
    lv_anim_set_playback_time(&anim_defrost_pulse, 800);
    lv_anim_set_repeat_count(&anim_defrost_pulse, LV_ANIM_REPEAT_INFINITE);
}

void hmi_update() {
    // This function is called periodically from the main lvgl task
    // It reads the global state and updates the UI elements
    xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
    
    // Update temperatures
    lv_label_set_text_fmt(label_actual_temp, "%.1f", g_system_state.actual_temp_celsius);
    lv_label_set_text_fmt(label_set_temp, "%.0f", g_system_state.setpoint_temp_celsius);
    
    xSemaphoreGive(g_system_state.mutex);
}

void hmi_trigger_alarm_animation(bool active) {
    if (active) {
        lv_anim_start(&anim_temp_pulse_color);
        lv_obj_clear_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
        lv_anim_start(&anim_silence_fade_in);
    } else {
        lv_anim_del(label_actual_temp, temp_color_anim_cb);
        lv_obj_remove_style(label_actual_temp, &style_actual_temp_alarm, 0);
        lv_obj_add_style(label_actual_temp, &style_actual_temp_normal, 0);
        lv_obj_add_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
    }
}

void hmi_set_alarm_acknowledged(bool acknowledged) {
    lv_anim_del(label_actual_temp, temp_color_anim_cb);
    lv_obj_add_flag(btn_silence, LV_OBJ_FLAG_HIDDEN);
    if (acknowledged) {
        lv_obj_add_style(label_actual_temp, &style_actual_temp_alarm, 0);
    } else {
        lv_obj_remove_style(label_actual_temp, &style_actual_temp_alarm, 0);
        lv_obj_add_style(label_actual_temp, &style_actual_temp_normal, 0);
    }
}

void hmi_trigger_defrost_animation(bool active) {
    if (active) {
        lv_anim_start(&anim_defrost_pulse);
    } else {
        lv_anim_del(btn_defrost, opacity_anim_cb);
        lv_obj_set_style_opa(btn_defrost, LV_OPA_COVER, 0);
    }
}

void hmi_update_fault_display(const char* fault_text) {
    if (fault_text != nullptr && strlen(fault_text) > 0) {
        lv_label_set_text(label_fault_display, fault_text);
        lv_obj_clear_flag(label_fault_display, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(label_set_temp, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(label_fault_display, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(label_set_temp, LV_OBJ_FLAG_HIDDEN);
    }
}