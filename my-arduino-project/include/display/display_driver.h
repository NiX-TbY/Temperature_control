#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include "config/config.h"
#include "types/types.h"

// Forward declarations
struct TouchEvent;

// Simple LovyanGFX placeholder class for now
class LGFX : public lgfx::LGFX_Device {
public:
    LGFX() { }
    bool init() { return true; }
    void fillScreen(uint16_t color) { }
    void setRotation(uint8_t r) { }
    void startWrite() { }
    void endWrite() { }
    void setAddrWindow(int32_t x, int32_t y, int32_t w, int32_t h) { }
    void writePixels(uint16_t* colors, int32_t len) { }
    bool getTouch(uint16_t* x, uint16_t* y) { return false; }
};

class DisplayDriver {
private:
    LGFX* _lgfx;
    bool _initialized;
    SemaphoreHandle_t _mutex;
    
    // LVGL objects
    lv_disp_draw_buf_t draw_buf;
    lv_disp_drv_t disp_drv;
    lv_disp_t* disp;
    lv_indev_drv_t indev_drv;
    lv_indev_t* indev;
    
    // Frame buffers in PSRAM
    lv_color_t* buf1;
    lv_color_t* buf2;
    
    // Display update flags
    bool _needsRedraw;
    unsigned long _lastUpdate;
    
    // Private initialization methods
    bool initI2C();
    bool initDisplay();
    bool initTouch();
    bool initLVGL();
    
    static void lvgl_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p);
    static void lvgl_touch_cb(lv_indev_drv_t* indev_drv, lv_indev_data_t* data);
    
public:
    DisplayDriver();
    ~DisplayDriver();
    
    bool init();
    void update();
    void setBrightness(uint8_t brightness);
    void clear();
    void sleep();
    void wake();
    
    // Thread-safe operations
    void lock() { xSemaphoreTake(_mutex, portMAX_DELAY); }
    void unlock() { xSemaphoreGive(_mutex); }
    
    // Getters
    bool isInitialized() const { return _initialized; }
    uint16_t getWidth() const { return DISPLAY_WIDTH; }
    uint16_t getHeight() const { return DISPLAY_HEIGHT; }
    LGFX* getLGFX() { return _lgfx; }
    LGFX* getTFT() { return _lgfx; }  // Alias for getLGFX
    
    // UI Methods
    bool getTouch(TouchEvent& event);
    void handleTouch(const TouchEvent& event);
    void addButton(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, uint16_t color, void (*callback)());
    void drawMainScreen(const SensorData& data, const SystemConfig& config, const ControlState& state);
    void forceRedraw() { _needsRedraw = true; }

};

// Global display instance
extern DisplayDriver display;

#endif // DISPLAY_DRIVER_H