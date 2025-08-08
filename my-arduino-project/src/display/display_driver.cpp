#include "display/display_driver.h"
#include "hal/ch422g.h"
#include "display/gt911.h"

// Global display instance
DisplayDriver display;

// Static callbacks
void DisplayDriver::lvgl_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    DisplayDriver* driver = (DisplayDriver*)disp_drv->user_data;
    if (driver && driver->_lgfx) {
        uint32_t w = (area->x2 - area->x1 + 1);
        uint32_t h = (area->y2 - area->y1 + 1);
        driver->_lgfx->startWrite();
        driver->_lgfx->setAddrWindow(area->x1, area->y1, w, h);
        driver->_lgfx->writePixels((uint16_t*)color_p, w * h);
        driver->_lgfx->endWrite();
    }
    lv_disp_flush_ready(disp_drv);
}

void DisplayDriver::lvgl_touch_cb(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    DisplayDriver* driver = (DisplayDriver*)indev_drv->user_data;
    if (driver) {
#ifdef ENABLE_TOUCH
        // Only poll controller if IRQ flagged (interrupt-driven) or periodic fallback every ~50ms.
        static uint32_t lastPoll = 0;
        bool shouldPoll = driver->_touchIRQFlag;
        uint32_t now = millis();
        if (!shouldPoll && (now - lastPoll) > 50) shouldPoll = true; // fallback
        if (shouldPoll) {
            driver->_touchIRQFlag = false; // clear flag
            uint16_t tx, ty; bool pressed=false;
            if (gt911.readTouch(tx, ty, pressed) && pressed) {
                data->state = LV_INDEV_STATE_PR;
                data->point.x = tx;
                data->point.y = ty;
                lastPoll = now;
                return;
            }
            lastPoll = now;
        }
#endif
        data->state = LV_INDEV_STATE_REL;
    }
}

DisplayDriver::DisplayDriver() {
    _lgfx = nullptr;
    buf1 = nullptr;
    buf2 = nullptr;
    disp = nullptr;
    indev = nullptr;
    _initialized = false;
    _needsRedraw = true;
    _lastUpdate = 0;
    _mutex = xSemaphoreCreateMutex();
}

DisplayDriver::~DisplayDriver() {
    if (_mutex) {
        vSemaphoreDelete(_mutex);
    }
    if (buf1) {
        heap_caps_free(buf1);
    }
    if (buf2) {
        heap_caps_free(buf2);
    }
    if (_lgfx) {
        delete _lgfx;
    }
}

bool DisplayDriver::init() {
    DEBUG_PRINTLN("=== Starting Waveshare Type B Display Initialization ===");
    if (!initI2C()) {
        DEBUG_PRINTLN("ERROR: I2C initialization failed!");
        return false;
    }
    if (!initDisplay()) {
        DEBUG_PRINTLN("ERROR: Display initialization failed!");
        return false;
    }
    if (!initTouch()) {
        DEBUG_PRINTLN("ERROR: Touch initialization failed!");
        return false;
    }
    if (!initLVGL()) {
        DEBUG_PRINTLN("ERROR: LVGL initialization failed!");
        return false;
    }
    _initialized = true;
    DEBUG_PRINTLN("=== Display Initialization Complete ===");
    return true;
}

bool DisplayDriver::initI2C() {
    DEBUG_PRINTLN("Step 1: Initializing I2C bus...");
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
    delay(100);
    DEBUG_PRINTLN("I2C bus initialized successfully");
    if (ch422g.begin(Wire)) {
        DEBUG_PRINTLN("CH422G expander detected");
        ch422g.setBacklight(100);
    } else {
        DEBUG_PRINTLN("WARNING: CH422G expander not detected");
    // Prepare PWM backlight fallback if dedicated GPIO available.
    ledcAttachPin(DISPLAY_DE_PIN, DISPLAY_BL_PWM_CHANNEL); // NOTE: placeholder pin mapping – adjust to real BL pin if differs from DE
    ledcSetup(DISPLAY_BL_PWM_CHANNEL, DISPLAY_BL_PWM_FREQ, DISPLAY_BL_PWM_RESOLUTION);
    ledcWrite(DISPLAY_BL_PWM_CHANNEL, 255);
    }
    return true;
}

bool DisplayDriver::initDisplay() {
    DEBUG_PRINTLN("Step 2: Initializing Display (LovyanGFX)...");
    if (ch422g.isPresent()) {
        ch422g.setDisplayEnable(false);
        ch422g.resetPanel();
    }
    _lgfx = new LGFX();
    if (!_lgfx) {
        DEBUG_PRINTLN("ERROR: LGFX allocation failed");
        return false;
    }
    if (!_lgfx->init()) {
        DEBUG_PRINTLN("ERROR: LovyanGFX initialization failed!");
        return false;
    }
    _lgfx->setRotation(0);
    _lgfx->fillScreen(0x0000);
#ifdef ENABLE_RGB_PANEL
    DEBUG_PRINTLN("RGB panel path active (ENABLE_RGB_PANEL)");
#else
    DEBUG_PRINTLN("Stub panel path active (ENABLE_RGB_PANEL disabled)");
#endif
    if (ch422g.isPresent()) {
        ch422g.setDisplayEnable(true);
        ch422g.setBacklight(100);
    }
    return true;
}

bool DisplayDriver::initTouch() {
    DEBUG_PRINTLN("Step 3: Initializing Touch Controller...");
#ifndef ENABLE_TOUCH
    DEBUG_PRINTLN("Touch disabled via feature flag");
    return true;
#else
    if (ch422g.isPresent()) {
        ch422g.resetTouch();
    }
    if (gt911.begin(Wire)) {
        DEBUG_PRINTLN("GT911 touch controller detected");
        // Attach interrupt if pin supports
        pinMode(TOUCH_IRQ_PIN, INPUT_PULLUP);
        attachInterrupt(TOUCH_IRQ_PIN, DisplayDriver::touch_isr_trampoline, FALLING);
    } else {
        DEBUG_PRINTLN("WARNING: GT911 touch controller not found");
    }
    DEBUG_PRINTLN("Touch controller initialization complete");
    return gt911.isPresent();
#endif
}

bool DisplayDriver::initLVGL() {
    DEBUG_PRINTLN("Step 4: Initializing LVGL...");
    lv_init();
    size_t buffer_size = DISPLAY_WIDTH * DISPLAY_HEIGHT / 10;
    buf1 = (lv_color_t*)heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!buf1) {
        DEBUG_PRINTLN("ERROR: Failed to allocate LVGL buffer 1 in PSRAM");
        return false;
    }
    buf2 = (lv_color_t*)heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!buf2) {
        DEBUG_PRINTLN("ERROR: Failed to allocate LVGL buffer 2 in PSRAM");
        heap_caps_free(buf1); buf1 = nullptr;
        return false;
    }
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_size);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = this;
    disp = lv_disp_drv_register(&disp_drv);
    if (!disp) {
        DEBUG_PRINTLN("ERROR: Failed to register LVGL display driver");
        return false;
    }
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_cb;
    indev_drv.user_data = this;
    indev = lv_indev_drv_register(&indev_drv);
    if (!indev) {
        DEBUG_PRINTLN("ERROR: Failed to register LVGL input device");
        return false;
    }
    DEBUG_PRINTLN("LVGL initialized successfully");
    return true;
}

void DisplayDriver::update() {
    if (!_initialized) return;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    lv_timer_handler();
    _lastUpdate = millis();
    _needsRedraw = false;
    _frameCount++;
    if (_lastFpsCalcMs == 0) _lastFpsCalcMs = _lastUpdate;
    if (_lastUpdate - _lastFpsCalcMs >= 1000) {
        _lastFps = (_frameCount * 1000.0f) / (_lastUpdate - _lastFpsCalcMs);
        _frameCount = 0;
        _lastFpsCalcMs = _lastUpdate;
    }
    xSemaphoreGive(_mutex);
}

void DisplayDriver::setBrightness(uint8_t brightness) {
    if (!_initialized || !_lgfx) return;
    DEBUG_PRINTF("Setting brightness to %d%%\n", brightness);
    if (ch422g.isPresent()) {
        ch422g.setBacklight(brightness);
    } else {
        uint8_t duty = map(brightness, 0, 100, 0, 255);
        ledcWrite(DISPLAY_BL_PWM_CHANNEL, duty);
    }
}

// Static ISR trampoline
void IRAM_ATTR DisplayDriver::touch_isr_trampoline() {
    // Retrieve global instance
    display.setTouchIRQFlag();
}

void DisplayDriver::handleTouchIRQ() {
    _touchIRQFlag = true;
}

void DisplayDriver::clear() {
    if (!_initialized || !_lgfx) return;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _lgfx->fillScreen(0x0000);
    xSemaphoreGive(_mutex);
}

void DisplayDriver::sleep() {
    if (!_initialized || !_lgfx) return;
    DEBUG_PRINTLN("Display entering sleep mode");
}

void DisplayDriver::wake() {
    if (!_initialized || !_lgfx) return;
    DEBUG_PRINTLN("Display waking up");
}
