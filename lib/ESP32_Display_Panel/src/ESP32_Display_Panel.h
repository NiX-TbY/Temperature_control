#pragma once

#include <Arduino.h>
#include "ESP_IOExpander.h"

namespace esp_panel {
namespace drivers {

struct RGB_Config {
    uint16_t width;
    uint16_t height;
    uint8_t de_pin;
    uint8_t vsync_pin;
    uint8_t hsync_pin;
    uint8_t pclk_pin;
    uint8_t data_pins[24];
    uint32_t pclk_freq_hz;
    uint16_t hsync_front_porch;
    uint16_t hsync_back_porch;
    uint16_t hsync_pulse_width;
    uint16_t vsync_front_porch;
    uint16_t vsync_back_porch;
    uint16_t vsync_pulse_width;
    bool hsync_idle_low;
    bool vsync_idle_low;
    bool de_idle_high;
    bool pclk_active_neg;
};

class LCD_ST7262 {
public:
    LCD_ST7262(const RGB_Config& rgb_config, ESP_IOExpander* io_expander);
    ~LCD_ST7262();
    
    bool init();
    bool begin();
    void reset();
    void enable();
    void disable();
    
    void drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* bitmap);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void setBacklight(bool enable);
    
    uint16_t getWidth() const { return _config.width; }
    uint16_t getHeight() const { return _config.height; }
    
private:
    RGB_Config _config;
    ESP_IOExpander* _io_expander;
    bool _initialized;
    
    // CH422G pin assignments for Type B board
    static constexpr uint8_t DISPLAY_RST_PIN = 0;
    static constexpr uint8_t DISPLAY_BL_PIN = 1;
    
    bool initRGBInterface();
    bool sendInitSequence();
}; 

} // namespace drivers
} // namespace esp_panel