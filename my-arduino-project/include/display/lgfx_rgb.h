// Real RGB panel configuration with timing abstraction and runtime adjustment.
#pragma once

#include <LovyanGFX.hpp>
#if __has_include(<lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>) && __has_include(<lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>)
  #include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
  #include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
  #define HAS_RGB_PANEL 1
#else
  #define HAS_RGB_PANEL 0
#endif

#include "config/config.h"
#include "config/feature_flags.h"
#include "display/display_pins.h"
#include "display/display_timing.h"

#ifdef ENABLE_RGB_PANEL

class LGFX : public lgfx::LGFX_Device {
public:
    LGFX() { init_impl(); }

    bool init() { return lgfx::LGFX_Device::init(); }

    bool applyTiming(const RgbPanelTiming &t) {
#if HAS_RGB_PANEL
        auto bcfg = _bus.config();
        bcfg.freq_write          = t.pclk_hz;
        bcfg.hsync_front_porch   = t.hsync_front_porch;
        bcfg.hsync_pulse_width   = t.hsync_pulse_width;
        bcfg.hsync_back_porch    = t.hsync_back_porch;
        bcfg.vsync_front_porch   = t.vsync_front_porch;
        bcfg.vsync_pulse_width   = t.vsync_pulse_width;
        bcfg.vsync_back_porch    = t.vsync_back_porch;
        bcfg.hsync_polarity      = t.hsync_active_low ? 0 : 1;
        bcfg.vsync_polarity      = t.vsync_active_low ? 0 : 1;
        bcfg.pclk_active_neg     = t.pclk_rising_edge ? 0 : 1;
        bcfg.de_idle_high        = t.de_active_high ? 0 : 1;
        _bus.config(bcfg);
        return true;
#else
        (void)t; return false;
#endif
    }

private:
#if HAS_RGB_PANEL
    lgfx::Panel_RGB _panel;
    lgfx::Bus_RGB   _bus;
#endif

    void init_impl() {
#if HAS_RGB_PANEL
        const RgbPanelTiming timing = DEFAULT_RGB_TIMING;
        auto bcfg = _bus.config();
        bcfg.freq_write     = timing.pclk_hz;
        bcfg.pin_d0  = LCD_PIN_B3;  bcfg.pin_d1  = LCD_PIN_B4;  bcfg.pin_d2  = LCD_PIN_B5;  bcfg.pin_d3  = LCD_PIN_B6;
        bcfg.pin_d4  = LCD_PIN_B7;  bcfg.pin_d5  = LCD_PIN_G2;  bcfg.pin_d6  = LCD_PIN_G3;  bcfg.pin_d7  = LCD_PIN_G4;
        bcfg.pin_d8  = LCD_PIN_G5;  bcfg.pin_d9  = LCD_PIN_G6;  bcfg.pin_d10 = LCD_PIN_G7;  bcfg.pin_d11 = LCD_PIN_R3;
        bcfg.pin_d12 = LCD_PIN_R4;  bcfg.pin_d13 = LCD_PIN_R5;  bcfg.pin_d14 = LCD_PIN_R6;  bcfg.pin_d15 = LCD_PIN_R7;
        bcfg.pin_henable = DISPLAY_DE_PIN;
        bcfg.pin_vsync   = DISPLAY_VSYNC_PIN;
        bcfg.pin_hsync   = DISPLAY_HSYNC_PIN;
        bcfg.pin_pclk    = DISPLAY_PCLK_PIN;
        bcfg.hsync_front_porch = timing.hsync_front_porch;
        bcfg.hsync_pulse_width = timing.hsync_pulse_width;
        bcfg.hsync_back_porch  = timing.hsync_back_porch;
        bcfg.vsync_front_porch = timing.vsync_front_porch;
        bcfg.vsync_pulse_width = timing.vsync_pulse_width;
        bcfg.vsync_back_porch  = timing.vsync_back_porch;
        bcfg.hsync_polarity = timing.hsync_active_low ? 0 : 1;
        bcfg.vsync_polarity = timing.vsync_active_low ? 0 : 1;
        bcfg.pclk_active_neg = timing.pclk_rising_edge ? 0 : 1;
        bcfg.de_idle_high = timing.de_active_high ? 0 : 1; // library expects idle level
        bcfg.pclk_idle_high = 0;
        _bus.config(bcfg);

        auto pcfg = _panel.config();
        pcfg.memory_width  = DISPLAY_WIDTH;
        pcfg.memory_height = DISPLAY_HEIGHT;
        pcfg.panel_width   = DISPLAY_WIDTH;
        pcfg.panel_height  = DISPLAY_HEIGHT;
        pcfg.offset_x = 0;
        pcfg.offset_y = 0;
        pcfg.readable = false;
        _panel.config(pcfg);

        _panel.setBus(&_bus);
        setPanel(&_panel);
#endif
    }
};

#endif // ENABLE_RGB_PANEL
