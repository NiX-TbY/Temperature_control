#include "display/ui_screens.h"
#include "controllers/temperature_controller.h"
#include "config/feature_flags.h"
#include "display/display_driver.h"
#ifdef ENABLE_DIAG_OVERLAY
#include <esp_heap_caps.h>
#endif

// Font fallbacks: map unavailable large Montserrat fonts to enabled ones to avoid build failures in minimal config.
#ifndef lv_font_montserrat_48
    #define lv_font_montserrat_48 lv_font_montserrat_16
#endif
#ifndef lv_font_montserrat_32
    #define lv_font_montserrat_32 lv_font_montserrat_16
#endif
#ifndef lv_font_montserrat_28
    #define lv_font_montserrat_28 lv_font_montserrat_16
#endif
#ifndef lv_font_montserrat_24
    #define lv_font_montserrat_24 lv_font_montserrat_16
#endif
#ifndef lv_font_montserrat_20
    #define lv_font_montserrat_20 lv_font_montserrat_16
#endif
#ifndef lv_font_montserrat_18
    #define lv_font_montserrat_18 lv_font_montserrat_16
#endif
#ifndef lv_font_montserrat_14
    #define lv_font_montserrat_14 lv_font_montserrat_16
#endif

// Global UI instance
UIScreens ui;

UIScreens::UIScreens() {
    mainScreen = nullptr;
    settingsScreen = nullptr;
    dataScreen = nullptr;
    currentScreen = nullptr;
    tempLabel = nullptr;
    targetTempLabel = nullptr;
    statusLabel = nullptr;
    timeLabel = nullptr;
    modeLabel = nullptr;
    compressorIcon = nullptr;
    fanIcon = nullptr;
    tempSlider = nullptr;
    tempSliderLabel = nullptr;
    modeDropdown = nullptr;
    fanSlider = nullptr;
    fanSliderLabel = nullptr;
    
    for (int i = 0; i < 4; i++) {
        sensorLabels[i] = nullptr;
    }
    faultOverlay = nullptr;
    faultLabel = nullptr;
    serviceMenuActive = false;
    hotspotPressedTime = 0;
    alarmZone = nullptr;
    alarmZoneLabel = nullptr;
#ifdef ENABLE_DIAG_OVERLAY
    diagLabel = nullptr;
    diagTimer = nullptr;
#endif
}

UIScreens::~UIScreens() {
    // LVGL objects are automatically cleaned up
}

bool UIScreens::init() {
    DEBUG_PRINTLN("Initializing UI Screens...");
    
    // Create styles
    createStyles();
    
    // Create screens
    createMainScreen();
    createSettingsScreen();
    createDataScreen();
    createFaultOverlay();
    createServiceHotspot();
#ifdef ENABLE_DIAG_OVERLAY
    createDiagOverlay();
#endif
    
    // Show main screen by default
    showMainScreen();
    
    DEBUG_PRINTLN("UI Screens initialized");
    return true;
}

#ifdef ENABLE_DIAG_OVERLAY
void UIScreens::createDiagOverlay() {
    diagLabel = lv_label_create(lv_layer_top());
    lv_obj_set_style_text_font(diagLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(diagLabel, lv_color_hex(0x00FF66), 0);
    lv_obj_align(diagLabel, LV_ALIGN_BOTTOM_LEFT, 5, -5);
    diagTimer = lv_timer_create([](lv_timer_t* t){
        UIScreens* ui = static_cast<UIScreens*>(t->user_data);
        if (!ui || !ui->diagLabel) return;
        float fps = display.getFPS();
        size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        static char buf[96];
        snprintf(buf, sizeof(buf), "FPS: %.1f Free: %u KB", fps, (unsigned)(freeHeap/1024));
        lv_label_set_text(ui->diagLabel, buf);
    }, 1000, this);
}
#endif

void UIScreens::createStyles() {
    // Main background style
    lv_style_init(&styleMain);
    lv_style_set_bg_color(&styleMain, lv_color_black());
    lv_style_set_bg_opa(&styleMain, LV_OPA_COVER);
    lv_style_set_border_width(&styleMain, 0);
    lv_style_set_pad_all(&styleMain, 0);
    
    // Large temperature display style
    lv_style_init(&styleTempLarge);
    lv_style_set_text_color(&styleTempLarge, lv_color_hex(0x00FFFF));  // Cyan
    lv_style_set_text_font(&styleTempLarge, &lv_font_montserrat_48);
    lv_style_set_text_align(&styleTempLarge, LV_TEXT_ALIGN_CENTER);
    
    // Button style
    lv_style_init(&styleButton);
    lv_style_set_bg_color(&styleButton, lv_color_hex(0x4169E1));  // Royal Blue
    lv_style_set_bg_opa(&styleButton, LV_OPA_COVER);
    lv_style_set_border_color(&styleButton, lv_color_hex(0x6495ED));
    lv_style_set_border_width(&styleButton, 2);
    lv_style_set_border_opa(&styleButton, LV_OPA_50);
    lv_style_set_radius(&styleButton, 10);
    lv_style_set_text_color(&styleButton, lv_color_white());
    
    // Button pressed style
    lv_style_init(&styleButtonPressed);
    lv_style_set_bg_color(&styleButtonPressed, lv_color_hex(0x1E90FF));
    lv_style_set_transform_width(&styleButtonPressed, -5);
    lv_style_set_transform_height(&styleButtonPressed, -5);
    
    // Sensor display style
    lv_style_init(&styleSensor);
    lv_style_set_bg_color(&styleSensor, lv_color_hex(0x1A1A1A));
    lv_style_set_bg_opa(&styleSensor, LV_OPA_80);
    lv_style_set_border_color(&styleSensor, lv_color_hex(0x4169E1));
    lv_style_set_border_width(&styleSensor, 1);
    lv_style_set_radius(&styleSensor, 8);
    lv_style_set_pad_all(&styleSensor, 10);
    lv_style_set_text_color(&styleSensor, lv_color_white());
}

void UIScreens::createMainScreen() {
    // Create main screen
    mainScreen = lv_obj_create(NULL);
    lv_obj_add_style(mainScreen, &styleMain, 0);
    
    // Large temperature display (takes up 3/4 of screen width)
    lv_obj_t* tempContainer = lv_obj_create(mainScreen);
    lv_obj_set_size(tempContainer, 600, 380);
    lv_obj_align(tempContainer, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(tempContainer, lv_color_black(), 0);
    lv_obj_set_style_border_width(tempContainer, 0, 0);
    
    // Main temperature label (huge font)
    tempLabel = lv_label_create(tempContainer);
    lv_label_set_text(tempLabel, "-18.2°C");
    lv_obj_set_style_text_font(tempLabel, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(tempLabel, lv_color_hex(0x00FFFF), 0);
    lv_obj_align(tempLabel, LV_ALIGN_CENTER, 0, -40);

    // Alarm zone (bottom-right area inside temp container for silence/countdown)
    alarmZone = lv_obj_create(tempContainer);
    lv_obj_set_size(alarmZone, 200, 80);
    lv_obj_align(alarmZone, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(alarmZone, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(alarmZone, LV_OPA_50, 0);
    lv_obj_set_style_border_width(alarmZone, 0, 0);
    lv_obj_add_flag(alarmZone, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(alarmZone, this);
    lv_obj_add_event_cb(alarmZone, [](lv_event_t* e){
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            UIScreens* ui = (UIScreens*)lv_obj_get_user_data(lv_event_get_current_target(e));
            if (!ui) return;
            auto state = controller.getState();
            if (state.alarmActive && !state.alarmSilenced) {
                controller.silenceAlarm();
                ui->showAlert("Alarm Silenced");
            }
        }
    }, LV_EVENT_CLICKED, nullptr);
    alarmZoneLabel = lv_label_create(alarmZone);
    lv_label_set_text(alarmZoneLabel, "");
    lv_obj_center(alarmZoneLabel);
    
    // Target temperature label
    targetTempLabel = lv_label_create(tempContainer);
    lv_label_set_text(targetTempLabel, "Target: -18.0°C");
    lv_obj_set_style_text_font(targetTempLabel, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(targetTempLabel, lv_color_hex(0x808080), 0);
    lv_obj_align(targetTempLabel, LV_ALIGN_CENTER, 0, 20);
    
    // Status label
    statusLabel = lv_label_create(tempContainer);
    lv_label_set_text(statusLabel, "COOLING");
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x00FF00), 0);
    lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, 80);
    
    // Right side panel for sensors
    lv_obj_t* sidePanel = lv_obj_create(mainScreen);
    lv_obj_set_size(sidePanel, 180, 460);
    lv_obj_align(sidePanel, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(sidePanel, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_border_width(sidePanel, 0, 0);
    lv_obj_set_flex_flow(sidePanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sidePanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(sidePanel, 10, 0);
    lv_obj_set_style_pad_all(sidePanel, 10, 0);
    
    // Time label (displaying current date and time)
    timeLabel = lv_label_create(mainScreen);
    lv_label_set_text(timeLabel, "2025-08-07 06:15:12");
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(timeLabel, lv_color_white(), 0);
    lv_obj_align(timeLabel, LV_ALIGN_TOP_LEFT, 20, 10);
    
    // Sensor displays
    for (int i = 0; i < 4; i++) {
        lv_obj_t* sensorBox = lv_obj_create(sidePanel);
        lv_obj_set_size(sensorBox, 160, 80);
        lv_obj_add_style(sensorBox, &styleSensor, 0);
        
        // Sensor ID label
        lv_obj_t* idLabel = lv_label_create(sensorBox);
        lv_label_set_text_fmt(idLabel, "Sensor %d", i + 1);
        lv_obj_set_style_text_font(idLabel, &lv_font_montserrat_14, 0);
        lv_obj_align(idLabel, LV_ALIGN_TOP_LEFT, 5, 5);
        
        // Temperature value
        sensorLabels[i] = lv_label_create(sensorBox);
        lv_label_set_text(sensorLabels[i], "--.-°C");
        lv_obj_set_style_text_font(sensorLabels[i], &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(sensorLabels[i], lv_color_hex(0x00FFFF), 0);
    lv_obj_align(sensorLabels[i], LV_ALIGN_BOTTOM_MID, 0, -5);
    }
    
    // Settings button
    lv_obj_t* settingsBtn = lv_btn_create(sidePanel);
    lv_obj_set_size(settingsBtn, 160, 50);
    lv_obj_add_style(settingsBtn, &styleButton, 0);
    lv_obj_add_style(settingsBtn, &styleButtonPressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(settingsBtn, settingsBtnEvent, LV_EVENT_CLICKED, this);
    
    lv_obj_t* settingsBtnLabel = lv_label_create(settingsBtn);
    lv_label_set_text(settingsBtnLabel, "Settings");
    lv_obj_center(settingsBtnLabel);
    
    // User label
    lv_obj_t* userLabel = lv_label_create(mainScreen);
    lv_label_set_text(userLabel, "NiX-TbY");
    lv_obj_set_style_text_font(userLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(userLabel, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(userLabel, LV_ALIGN_TOP_RIGHT, -20, 10);
}

void UIScreens::createFaultOverlay() {
    faultOverlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(faultOverlay, LV_PCT(100), 60);
    lv_obj_align(faultOverlay, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(faultOverlay, lv_color_hex(0x550000), 0);
    lv_obj_set_style_bg_opa(faultOverlay, LV_OPA_80, 0);
    lv_obj_set_style_border_width(faultOverlay, 0, 0);
    lv_obj_add_flag(faultOverlay, LV_OBJ_FLAG_HIDDEN);

    faultLabel = lv_label_create(faultOverlay);
    lv_obj_set_width(faultLabel, LV_PCT(100));
    lv_label_set_long_mode(faultLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(faultLabel, "NO FAULTS ACTIVE");
    lv_obj_set_style_text_color(faultLabel, lv_color_hex(0xFFCCCC), 0);
    lv_obj_set_style_text_font(faultLabel, &lv_font_montserrat_20, 0);
    lv_obj_align(faultLabel, LV_ALIGN_CENTER, 0, 0);
}

void UIScreens::createServiceHotspot() {
    // Transparent hotspot in top-right corner (100x100)
    serviceHotspot = lv_btn_create(mainScreen);
    lv_obj_set_size(serviceHotspot, 120, 120);
    lv_obj_align(serviceHotspot, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_opa(serviceHotspot, LV_OPA_0, 0);
    lv_obj_set_style_border_width(serviceHotspot, 0, 0);
    lv_obj_add_flag(serviceHotspot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(serviceHotspot, serviceHotspotEvent, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(serviceHotspot, serviceHotspotEvent, LV_EVENT_RELEASED, this);
    lv_obj_add_event_cb(serviceHotspot, serviceHotspotEvent, LV_EVENT_PRESSING, this);
}

void UIScreens::createSettingsScreen() {
    // Create settings screen
    settingsScreen = lv_obj_create(NULL);
    lv_obj_add_style(settingsScreen, &styleMain, 0);
    
    // Title
    lv_obj_t* title = lv_label_create(settingsScreen);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Temperature control
    lv_obj_t* tempControl = lv_obj_create(settingsScreen);
    lv_obj_set_size(tempControl, 700, 100);
    lv_obj_align(tempControl, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_color(tempControl, lv_color_hex(0x1A1A1A), 0);
    
    lv_obj_t* tempTitle = lv_label_create(tempControl);
    lv_label_set_text(tempTitle, "Target Temperature");
    lv_obj_align(tempTitle, LV_ALIGN_TOP_LEFT, 20, 10);
    
    tempSlider = lv_slider_create(tempControl);
    lv_obj_set_size(tempSlider, 500, 20);
    lv_obj_align(tempSlider, LV_ALIGN_CENTER, 0, 10);
    lv_slider_set_range(tempSlider, -30, 0);
    lv_slider_set_value(tempSlider, -18, LV_ANIM_OFF);
    lv_obj_add_event_cb(tempSlider, tempSliderEvent, LV_EVENT_VALUE_CHANGED, this);
    
    tempSliderLabel = lv_label_create(tempControl);
    lv_label_set_text(tempSliderLabel, "-18°C");
    lv_obj_align(tempSliderLabel, LV_ALIGN_RIGHT_MID, -20, 10);
    
    // Mode selection
    lv_obj_t* modeControl = lv_obj_create(settingsScreen);
    lv_obj_set_size(modeControl, 700, 100);
    lv_obj_align(modeControl, LV_ALIGN_TOP_MID, 0, 200);
    lv_obj_set_style_bg_color(modeControl, lv_color_hex(0x1A1A1A), 0);
    
    lv_obj_t* modeTitle = lv_label_create(modeControl);
    lv_label_set_text(modeTitle, "Operating Mode");
    lv_obj_align(modeTitle, LV_ALIGN_TOP_LEFT, 20, 10);
    
    modeDropdown = lv_dropdown_create(modeControl);
    lv_dropdown_set_options(modeDropdown, "Auto\nManual Cool\nManual Heat\nDefrost\nOff");
    lv_obj_set_size(modeDropdown, 200, 40);
    lv_obj_align(modeDropdown, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_event_cb(modeDropdown, modeDropdownEvent, LV_EVENT_VALUE_CHANGED, this);
    
    // Fan speed control
    lv_obj_t* fanControl = lv_obj_create(settingsScreen);
    lv_obj_set_size(fanControl, 700, 100);
    lv_obj_align(fanControl, LV_ALIGN_TOP_MID, 0, 320);
    lv_obj_set_style_bg_color(fanControl, lv_color_hex(0x1A1A1A), 0);
    
    lv_obj_t* fanTitle = lv_label_create(fanControl);
    lv_label_set_text(fanTitle, "Fan Speed");
    lv_obj_align(fanTitle, LV_ALIGN_TOP_LEFT, 20, 10);
    
    fanSlider = lv_slider_create(fanControl);
    lv_obj_set_size(fanSlider, 500, 20);
    lv_obj_align(fanSlider, LV_ALIGN_CENTER, 0, 10);
    lv_slider_set_range(fanSlider, 0, 100);
    lv_slider_set_value(fanSlider, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(fanSlider, fanSliderEvent, LV_EVENT_VALUE_CHANGED, this);
    
    fanSliderLabel = lv_label_create(fanControl);
    lv_label_set_text(fanSliderLabel, "50%");
    lv_obj_align(fanSliderLabel, LV_ALIGN_RIGHT_MID, -20, 10);
    
    // Back button
    lv_obj_t* backBtn = lv_btn_create(settingsScreen);
    lv_obj_set_size(backBtn, 120, 50);
    lv_obj_align(backBtn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_style(backBtn, &styleButton, 0);
    lv_obj_add_style(backBtn, &styleButtonPressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(backBtn, backBtnEvent, LV_EVENT_CLICKED, this);
    
    lv_obj_t* backBtnLabel = lv_label_create(backBtn);
    lv_label_set_text(backBtnLabel, "Back");
    lv_obj_center(backBtnLabel);
}

void UIScreens::createDataScreen() {
    // Create data logging screen
    dataScreen = lv_obj_create(NULL);
    lv_obj_add_style(dataScreen, &styleMain, 0);
    
    // Title
    lv_obj_t* title = lv_label_create(dataScreen);
    lv_label_set_text(title, "Data Log");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Data log table
    lv_obj_t* table = lv_table_create(dataScreen);
    lv_obj_set_size(table, 740, 360);
    lv_obj_align(table, LV_ALIGN_CENTER, 0, 20);
    lv_table_set_col_cnt(table, 5);
    lv_table_set_row_cnt(table, 10);
    
    // Set column widths
    lv_table_set_col_width(table, 0, 200);  // Time
    lv_table_set_col_width(table, 1, 120);  // Sensor 1
    lv_table_set_col_width(table, 2, 120);  // Sensor 2
    lv_table_set_col_width(table, 3, 120);  // Sensor 3
    lv_table_set_col_width(table, 4, 120);  // Sensor 4
    
    // Set headers
    lv_table_set_cell_value(table, 0, 0, "Time");
    lv_table_set_cell_value(table, 0, 1, "Sensor 1");
    lv_table_set_cell_value(table, 0, 2, "Sensor 2");
    lv_table_set_cell_value(table, 0, 3, "Sensor 3");
    lv_table_set_cell_value(table, 0, 4, "Sensor 4");
    
    // Sample data (would be populated dynamically)
    lv_table_set_cell_value(table, 1, 0, "2025-08-07 06:15:12");
    lv_table_set_cell_value(table, 1, 1, "-18.2°C");
    lv_table_set_cell_value(table, 1, 2, "-18.5°C");
    lv_table_set_cell_value(table, 1, 3, "-17.9°C");
    lv_table_set_cell_value(table, 1, 4, "-18.3°C");
    
    // Back button
    lv_obj_t* backBtn = lv_btn_create(dataScreen);
    lv_obj_set_size(backBtn, 120, 50);
    lv_obj_align(backBtn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_style(backBtn, &styleButton, 0);
    lv_obj_add_style(backBtn, &styleButtonPressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(backBtn, backBtnEvent, LV_EVENT_CLICKED, this);
    
    lv_obj_t* backBtnLabel = lv_label_create(backBtn);
    lv_label_set_text(backBtnLabel, "Back");
    lv_obj_center(backBtnLabel);
}

void UIScreens::update(const SystemData& data) {
    if (!currentScreen) return;
    
    // Update main temperature display
    lv_label_set_text_fmt(tempLabel, "%.1f°C", data.control.currentTemp);
    lv_label_set_text_fmt(targetTempLabel, "Target: %.1f°C", data.config.targetTemp);
    
    // Set temperature label color based on state
    lv_color_t tempColor;
    if (data.control.currentTemp < -25.0) {
    tempColor = lv_color_hex(0x00BFFF); // Deep blue for very cold
    } else if (data.control.currentTemp < -15.0) {
    tempColor = lv_color_hex(0x00FFFF); // Cyan for normal freezer temp
    } else if (data.control.currentTemp < -5.0) {
    tempColor = lv_color_hex(0x00FF00); // Green for cool
    } else if (data.control.currentTemp < 5.0) {
    tempColor = lv_color_hex(0xFFFF00); // Yellow for warning
    } else {
    tempColor = lv_color_hex(0xFF0000); // Red for too warm
    }
    lv_obj_set_style_text_color(tempLabel, tempColor, 0);
    
    // Update status text
    const char* statusText = "UNKNOWN";
    lv_color_t statusColor = lv_color_hex(0xAAAAAA);
    
    switch (data.control.status) {
        case STATUS_IDLE:
            statusText = "IDLE";
            statusColor = lv_color_hex(0x00FFFF);
            break;
        case STATUS_COOLING:
            statusText = "COOLING";
            statusColor = lv_color_hex(0x00BFFF);
            break;
        case STATUS_HEATING:
            statusText = "HEATING";
            statusColor = lv_color_hex(0xFF8000);
            break;
        case STATUS_DEFROST:
            statusText = "DEFROST";
            statusColor = lv_color_hex(0xFFFF00);
            break;
        case STATUS_ERROR:
            statusText = "ERROR";
            statusColor = lv_color_hex(0xFF0000);
            break;
    }
    
    lv_label_set_text(statusLabel, statusText);
    lv_obj_set_style_text_color(statusLabel, statusColor, 0);
    
    // Update time
    if (data.timeString.length() > 0) {
        lv_label_set_text(timeLabel, data.timeString.c_str());
    }
    
    // Update sensor data
    for (uint8_t i = 0; i < data.activeSensors && i < 4; i++) {
        if (data.sensors[i].valid) {
            lv_label_set_text_fmt(sensorLabels[i], "%.1f°C", data.sensors[i].temperature);
        } else {
            lv_label_set_text(sensorLabels[i], "--.-°C");
        }
    }

    // Fault overlay update (non-blocking)
    updateFaultOverlay(data);
    updateAlarmVisuals(data);
}

void UIScreens::updateFaultOverlay(const SystemData& data) {
    if (!faultOverlay || !faultLabel) return;
    if (data.control.faultMask == 0 && data.control.status != STATUS_ERROR) {
        lv_obj_add_flag(faultOverlay, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    static char buf[160];
    size_t pos = 0;
    auto appendToken = [&](const char* t){
        size_t l = strlen(t);
        if (pos + l + 1 < sizeof(buf)) { memcpy(buf+pos, t, l); pos += l; buf[pos++]=' '; }
    };
    if (data.control.faultMask == 0 && data.control.status == STATUS_ERROR) {
        strncpy(buf, "SYSTEM ERROR", sizeof(buf));
        pos = strlen(buf);
    } else {
        if (data.control.faultMask & FaultBit(FAULT_SENSOR_MISSING_BIT)) appendToken("Sensor Missing");
        if (data.control.faultMask & FaultBit(FAULT_SENSOR_RANGE_BIT)) appendToken("Sensor Range");
        if (data.control.faultMask & FaultBit(FAULT_OVER_TEMPERATURE_BIT)) appendToken("Over Temp");
        if (data.control.faultMask & FaultBit(FAULT_UNDER_TEMPERATURE_BIT)) appendToken("Under Temp");
        if (data.control.faultMask & FaultBit(FAULT_DEFROST_TIMEOUT_BIT)) appendToken("Defrost Timeout");
        if (data.control.faultMask & FaultBit(FAULT_COMPRESSOR_SHORT_CYCLE_BIT)) appendToken("Short Cycle");
        if (data.control.faultMask & FaultBit(FAULT_FAN_FAILURE_BIT)) appendToken("Fan Failure");
    }
    if (pos == 0) {
        strncpy(buf, "FAULT", sizeof(buf));
        pos = 5;
    }
    buf[pos] = '\0';
    lv_label_set_text(faultLabel, buf);
    lv_obj_clear_flag(faultOverlay, LV_OBJ_FLAG_HIDDEN);
}

void UIScreens::updateAlarmVisuals(const SystemData& data) {
    if (!alarmZone || !alarmZoneLabel) return;
    if (data.control.alarmActive) {
        if (!data.control.alarmSilenced) {
            unsigned long phase = (millis() / (ALARM_PULSE_INTERVAL_MS / 2)) % 2;
            lv_color_t c = phase ? lv_color_hex(0xFF0000) : lv_color_hex(0x800000);
            lv_obj_set_style_text_color(tempLabel, c, 0);
            lv_label_set_text(alarmZoneLabel, "SILENCE");
            lv_obj_set_style_text_color(alarmZoneLabel, lv_color_hex(0xFF0000), 0);
        } else {
            lv_obj_set_style_text_color(tempLabel, lv_color_hex(0xFF0000), 0);
            long remaining = (long)(data.control.alarmSilenceUntil - millis());
            if (remaining < 0) remaining = 0;
            int minutes = remaining / 60000;
            int seconds = (remaining / 1000) % 60;
            static char tbuf[16];
            snprintf(tbuf, sizeof(tbuf), "%02d:%02d", minutes, seconds);
            lv_label_set_text(alarmZoneLabel, tbuf);
            lv_obj_set_style_text_color(alarmZoneLabel, lv_color_white(), 0);
        }
    } else {
        // Clear alarm visuals (retain current temp label color set earlier by state logic)
        lv_label_set_text(alarmZoneLabel, "");
    }
}

void UIScreens::serviceHotspotEvent(lv_event_t* e) {
    UIScreens* ui = (UIScreens*)lv_event_get_user_data(e);
    if (!ui) return;
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        ui->hotspotPressedTime = millis();
    } else if (code == LV_EVENT_PRESSING) {
        if (!ui->serviceMenuActive && millis() - ui->hotspotPressedTime >= 5000) {
            ui->serviceMenuActive = true;
            ui->showSettingsScreen(); // Reuse settings screen as service menu placeholder
            ui->showAlert("Service Menu");
        }
    } else if (code == LV_EVENT_RELEASED) {
        ui->hotspotPressedTime = 0;
        // Keep serviceMenuActive true until user leaves
    }
}

void UIScreens::showMainScreen() {
    if (mainScreen) {
        lv_scr_load(mainScreen);
        currentScreen = mainScreen;
    }
}

void UIScreens::showSettingsScreen() {
    if (settingsScreen) {
        lv_scr_load(settingsScreen);
        currentScreen = settingsScreen;
        
        // Update controls with current values
        SystemConfig config = controller.getConfig();
        
        // Update temperature slider
        lv_slider_set_value(tempSlider, (int)config.targetTemp, LV_ANIM_OFF);
        lv_label_set_text_fmt(tempSliderLabel, "%.1f°C", config.targetTemp);
        
        // Update mode dropdown
        lv_dropdown_set_selected(modeDropdown, config.mode);
        
        // Update fan slider
        lv_slider_set_value(fanSlider, config.fanSpeed, LV_ANIM_OFF);
        lv_label_set_text_fmt(fanSliderLabel, "%d%%", config.fanSpeed);
    }
}

void UIScreens::showDataScreen() {
    if (dataScreen) {
        lv_scr_load(dataScreen);
        currentScreen = dataScreen;
    }
}

void UIScreens::showErrorScreen(const char* error) {
    // Create a simple modal dialog with error message
    lv_obj_t* modalBack = lv_obj_create(lv_layer_top());
    lv_obj_set_size(modalBack, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(modalBack, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modalBack, LV_OPA_70, 0);
    
    lv_obj_t* errorBox = lv_obj_create(modalBack);
    lv_obj_set_size(errorBox, 500, 300);
    lv_obj_center(errorBox);
    lv_obj_set_style_bg_color(errorBox, lv_color_hex(0x300000), 0);
    lv_obj_set_style_border_color(errorBox, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(errorBox, 2, 0);
    lv_obj_set_style_radius(errorBox, 10, 0);
    
    lv_obj_t* errorTitle = lv_label_create(errorBox);
    lv_label_set_text(errorTitle, "SYSTEM ERROR");
    lv_obj_set_style_text_color(errorTitle, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_text_font(errorTitle, &lv_font_montserrat_24, 0);
    lv_obj_align(errorTitle, LV_ALIGN_TOP_MID, 0, 20);
    
    lv_obj_t* errorMsg = lv_label_create(errorBox);
    lv_label_set_text(errorMsg, error);
    lv_obj_set_style_text_color(errorMsg, lv_color_white(), 0);
    lv_obj_set_style_text_font(errorMsg, &lv_font_montserrat_18, 0);
    lv_label_set_long_mode(errorMsg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(errorMsg, 460);
    lv_obj_align(errorMsg, LV_ALIGN_CENTER, 0, 0);
    
    lv_obj_t* okBtn = lv_btn_create(errorBox);
    lv_obj_set_size(okBtn, 100, 40);
    lv_obj_align(okBtn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(okBtn, lv_color_hex(0xAA0000), 0);
    
    lv_obj_t* okLabel = lv_label_create(okBtn);
    lv_label_set_text(okLabel, "OK");
    lv_obj_center(okLabel);
    
    lv_obj_add_event_cb(okBtn, [](lv_event_t* e) {
        lv_obj_t* target = lv_event_get_current_target(e);
        lv_obj_t* parent = lv_obj_get_parent(lv_obj_get_parent(target));
        lv_obj_del(parent);
    }, LV_EVENT_CLICKED, NULL);
}

void UIScreens::showAlert(const char* message) {
    // Create a simple toast/alert message
    lv_obj_t* toast = lv_obj_create(lv_layer_top());
    lv_obj_set_size(toast, 400, 80);
    lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(toast, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(toast, LV_OPA_80, 0);
    lv_obj_set_style_radius(toast, 10, 0);
    
    lv_obj_t* msg = lv_label_create(toast);
    lv_label_set_text(msg, message);
    lv_obj_center(msg);
    
    // Auto close after 3 seconds
    lv_timer_t* timer = lv_timer_create([](lv_timer_t* t) {
        lv_obj_del((lv_obj_t*)t->user_data);
        lv_timer_del(t);
    }, 3000, toast);
}

// Event handlers
void UIScreens::settingsBtnEvent(lv_event_t* e) {
    UIScreens* ui = (UIScreens*)lv_event_get_user_data(e);
    if (ui) {
        ui->showSettingsScreen();
    }
}

void UIScreens::backBtnEvent(lv_event_t* e) {
    UIScreens* ui = (UIScreens*)lv_event_get_user_data(e);
    if (ui) {
        ui->showMainScreen();
    }
}

void UIScreens::tempSliderEvent(lv_event_t* e) {
    UIScreens* ui = (UIScreens*)lv_event_get_user_data(e);
    if (!ui) return;
    
    lv_obj_t* slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    
    // Update label
    lv_label_set_text_fmt(ui->tempSliderLabel, "%d°C", value);
    
    // Only update controller after user finishes sliding
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        controller.setTargetTemperature(value);
    }
}

void UIScreens::modeDropdownEvent(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    
    lv_obj_t* dropdown = lv_event_get_target(e);
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    
    // Convert dropdown selection to SystemMode
    SystemMode mode = MODE_OFF;
    switch (selected) {
        case 0: mode = MODE_AUTO; break;
        case 1: mode = MODE_MANUAL_COOL; break;
        case 2: mode = MODE_MANUAL_HEAT; break;
        case 3: mode = MODE_DEFROST; break;
        case 4: mode = MODE_OFF; break;
        default: mode = MODE_AUTO; break;
    }
    
    controller.setMode(mode);
}

void UIScreens::fanSliderEvent(lv_event_t* e) {
    UIScreens* ui = (UIScreens*)lv_event_get_user_data(e);
    if (!ui) return;
    
    lv_obj_t* slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    
    // Update label
    lv_label_set_text_fmt(ui->fanSliderLabel, "%d%%", value);
    
    // Only update controller after user finishes sliding
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        controller.setFanSpeed(value);
    }
}