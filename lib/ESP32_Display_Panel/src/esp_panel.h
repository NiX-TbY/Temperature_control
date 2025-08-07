#pragma once

#include <Arduino.h>
#include "ESP_IOExpander.h"

namespace esp_panel {

namespace drivers {

struct BusRGB {
    struct Config {
        uint8_t de_pin;
        uint8_t vsync_pin;
        uint8_t hsync_pin;
        uint8_t pclk_pin;
        uint8_t data_pins[24]; // RGB888: 8R + 8G + 8B
    };
};

class LCD {
public:
    struct Config {
        uint16_t width;
        uint16_t height;
        uint8_t bits_per_pixel;
        uint32_t pixel_clock_hz;
        bool invert_color;
    };
    
    virtual ~LCD() = default;
    virtual bool init() = 0;
    virtual bool begin() = 0;
    virtual void drawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* bitmap) = 0;
};

class LCD_ST7262 : public LCD {
public:
    LCD_ST7262(const BusRGB::Config& bus_config, const LCD::Config& lcd_config);
    virtual ~LCD_ST7262();
    
    bool init() override;
    bool begin() override;
    void drawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* bitmap) override;

private:
    BusRGB::Config _bus_config;
    LCD::Config _lcd_config;
    bool _initialized;
    
    bool initRGBBus();
    bool sendInitCommands();
};

} // namespace drivers
} // namespace esp_panel