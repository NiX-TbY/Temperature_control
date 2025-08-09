#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include <lvgl.h>
#include "types/types.h"
#include "config/config.h"
#include "config/feature_flags.h"

class UIScreens {
private:
    // Screen objects
    lv_obj_t* mainScreen;
    lv_obj_t* settingsScreen;
    lv_obj_t* dataScreen;
    lv_obj_t* currentScreen;
    
    // Main screen widgets
    lv_obj_t* tempLabel;
    lv_obj_t* targetTempLabel;
    lv_obj_t* statusLabel;
    lv_obj_t* sensorLabels[4];
    lv_obj_t* timeLabel;
    lv_obj_t* modeLabel;
    lv_obj_t* compressorIcon;
    lv_obj_t* fanIcon;
    
    // Settings screen widgets
    lv_obj_t* tempSlider;
    lv_obj_t* tempSliderLabel;
    lv_obj_t* modeDropdown;
    lv_obj_t* fanSlider;
    lv_obj_t* fanSliderLabel;
    
    // Style objects
    lv_style_t styleMain;
    lv_style_t styleTempLarge;
    lv_style_t styleButton;
    lv_style_t styleButtonPressed;
    lv_style_t styleSensor;

    // Fault overlay & service hotspot
    lv_obj_t* faultOverlay;
    lv_obj_t* faultLabel;
    lv_obj_t* serviceHotspot;
    unsigned long hotspotPressedTime;
    bool serviceMenuActive;
    lv_obj_t* alarmZone;
    lv_obj_t* alarmZoneLabel;
    
#ifdef ENABLE_DIAG_OVERLAY
    lv_obj_t* diagLabel;
    lv_timer_t* diagTimer;
    void createDiagOverlay();
#endif

    void createFaultOverlay();
    void createServiceHotspot();
    void updateFaultOverlay(const SystemData& data);
    static void serviceHotspotEvent(lv_event_t* e);
    void updateAlarmVisuals(const SystemData& data);
    
    void createStyles();
    void createMainScreen();
    void createSettingsScreen();
    void createDataScreen();
    
    // Event handlers
    static void settingsBtnEvent(lv_event_t* e);
    static void backBtnEvent(lv_event_t* e);
    static void tempSliderEvent(lv_event_t* e);
    static void modeDropdownEvent(lv_event_t* e);
    static void fanSliderEvent(lv_event_t* e);
    
public:
    UIScreens();
    ~UIScreens();
    
    bool init();
    void update(const SystemData& data);
    void showMainScreen();
    void showSettingsScreen();
    void showDataScreen();
    void showErrorScreen(const char* error);
    void showAlert(const char* message);
    
    lv_obj_t* getCurrentScreen() { return currentScreen; }
};

// Global UI instance
extern UIScreens ui;

#endif // UI_SCREENS_H