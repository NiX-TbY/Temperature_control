#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "config/feature_flags.h"
#ifdef ENABLE_RGB_PANEL
#include "display/lgfx_rgb.h"
#else
#include <LovyanGFX.hpp>
#endif
#include <lvgl.h>
#include "config/config.h"
#include "types/types.h"

// Forward declarations
struct TouchEvent;

#ifndef ENABLE_RGB_PANEL
// Stub LGFX class (no hardware) used when RGB panel disabled.
class LGFX : public lgfx::LGFX_Device {
public:
    LGFX() {}
    bool init() { return true; }
    void fillScreen(uint16_t) {}
    void setRotation(uint8_t) {}
    void startWrite() {}
    void endWrite() {}
    void setAddrWindow(int32_t, int32_t, int32_t, int32_t) {}
    void writePixels(uint16_t*, int32_t) {}
    bool getTouch(uint16_t*, uint16_t*) { return false; }
};
#endif

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
    // Frame buffers
    lv_color_t* buf1;
    lv_color_t* buf2;
    bool _needsRedraw;
    unsigned long _lastUpdate;
    volatile bool _touchIRQFlag {false};
    // Performance metrics
    uint32_t _frameCount {0};
    uint32_t _lastFpsCalcMs {0};
    float _lastFps {0.0f};
    static void IRAM_ATTR touch_isr_trampoline();
    void handleTouchIRQ();

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

    void lock() { xSemaphoreTake(_mutex, portMAX_DELAY); }
    void unlock() { xSemaphoreGive(_mutex); }

    bool isInitialized() const { return _initialized; }
    uint16_t getWidth() const { return DISPLAY_WIDTH; }
    uint16_t getHeight() const { return DISPLAY_HEIGHT; }
    LGFX* getLGFX() { return _lgfx; }
    LGFX* getTFT() { return _lgfx; }

    bool getTouch(TouchEvent& event);
    void handleTouch(const TouchEvent& event);
    void addButton(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, uint16_t color, void (*callback)());
    void drawMainScreen(const SensorData& data, const SystemConfig& config, const ControlState& state);
    void forceRedraw() { _needsRedraw = true; }
    // For ISR to set flag
    void setTouchIRQFlag() { _touchIRQFlag = true; }
    float getFPS() const { return _lastFps; }
    uint32_t getFrameCount() const { return _frameCount; }
};

extern DisplayDriver display;

#endif // DISPLAY_DRIVER_H