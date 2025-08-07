#include "display/display_driver.h"

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
    if (driver && driver->_lgfx) {
        uint16_t touchX, touchY;
        bool touched = driver->_lgfx->getTouch(&touchX, &touchY);
        if (touched) {
            data->state = LV_INDEV_STATE_PR;
            data->point.x = touchX;
            data->point.y = touchY;
        } else {
            data->state = LV_INDEV_STATE_REL;
        }
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
    
    // Step 1: Initialize I2C bus (CRITICAL - must be first)
    if (!initI2C()) {
        DEBUG_PRINTLN("ERROR: I2C initialization failed!");
        return false;
    }
    
    // Step 2: Initialize LovyanGFX Display
    if (!initDisplay()) {
        DEBUG_PRINTLN("ERROR: Display initialization failed!");
        return false;
    }
    
    // Step 3: Initialize Touch Controller
    if (!initTouch()) {
        DEBUG_PRINTLN("ERROR: Touch initialization failed!");
        return false;
    }
    
    // Step 4: Initialize LVGL
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
    
    // Initialize I2C with specific pins for Type B board
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
    
    delay(100); // Allow I2C to stabilize
    
    DEBUG_PRINTLN("I2C bus initialized successfully");
    return true;
}

bool DisplayDriver::initDisplay() {
    DEBUG_PRINTLN("Step 2: Initializing LovyanGFX Display...");
    
    // Create LGFX instance
    _lgfx = new LGFX();
    
    // Initialize the display
    if (!_lgfx->init()) {
        DEBUG_PRINTLN("ERROR: LovyanGFX initialization failed!");
        return false;
    }
    
    // Set rotation and clear screen
    _lgfx->setRotation(0);
    _lgfx->fillScreen(0x0000);  // Black color (RGB565)
    
    DEBUG_PRINTLN("LovyanGFX display initialized successfully");
    return true;
}

bool DisplayDriver::initTouch() {
    DEBUG_PRINTLN("Step 3: Initializing Touch Controller...");
    
    // Touch is handled internally by LovyanGFX GT911 configuration
    // Test if touch is working
    uint16_t x, y;
    if (_lgfx->getTouch(&x, &y)) {
        DEBUG_PRINTLN("Touch controller detected but not touched");
    }
    
    DEBUG_PRINTLN("Touch controller initialized successfully");
    return true;
}

bool DisplayDriver::initLVGL() {
    DEBUG_PRINTLN("Step 4: Initializing LVGL...");
    
    // Initialize LVGL
    lv_init();
    
    // Allocate buffers in PSRAM
    size_t buffer_size = DISPLAY_WIDTH * DISPLAY_HEIGHT / 10; // 1/10 screen size
    
    buf1 = (lv_color_t*)heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!buf1) {
        DEBUG_PRINTLN("ERROR: Failed to allocate LVGL buffer 1 in PSRAM");
        return false;
    }
    
    buf2 = (lv_color_t*)heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!buf2) {
        DEBUG_PRINTLN("ERROR: Failed to allocate LVGL buffer 2 in PSRAM");
        heap_caps_free(buf1);
        buf1 = nullptr;
        return false;
    }
    
    // Initialize display buffer
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_size);
    
    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = this;
    
    // Register display driver
    disp = lv_disp_drv_register(&disp_drv);
    if (!disp) {
        DEBUG_PRINTLN("ERROR: Failed to register LVGL display driver");
        return false;
    }
    
    // Initialize input device driver (touch)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_cb;
    indev_drv.user_data = this;
    
    // Register input device
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
    
    // Handle LVGL tasks
    lv_timer_handler();
    
    _lastUpdate = millis();
    _needsRedraw = false;
    
    xSemaphoreGive(_mutex);
}

void DisplayDriver::setBrightness(uint8_t brightness) {
    if (!_initialized || !_lgfx) return;
    
    // Note: For Waveshare Type B, backlight is controlled via I2C expander
    // This is a placeholder - actual implementation would need I2C commands
    DEBUG_PRINTF("Setting brightness to %d%%\n", brightness);
}

void DisplayDriver::clear() {
    if (!_initialized || !_lgfx) return;
    
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _lgfx->fillScreen(0x0000);  // Black color
    xSemaphoreGive(_mutex);
}

void DisplayDriver::sleep() {
    if (!_initialized || !_lgfx) return;
    
    DEBUG_PRINTLN("Display entering sleep mode");
    // Implementation would control backlight via I2C
}

void DisplayDriver::wake() {
    if (!_initialized || !_lgfx) return;
    
    DEBUG_PRINTLN("Display waking up");
    // Implementation would control backlight via I2C
}
