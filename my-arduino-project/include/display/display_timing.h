#pragma once
// Centralized RGB panel timing & polarity configuration.
// Adjust after validating with oscilloscope / panel datasheet.

#include <stdint.h>

struct RgbPanelTiming {
    uint16_t width;
    uint16_t height;
    uint16_t hsync_front_porch;
    uint16_t hsync_pulse_width;
    uint16_t hsync_back_porch;
    uint16_t vsync_front_porch;
    uint16_t vsync_pulse_width;
    uint16_t vsync_back_porch;
    uint32_t pclk_hz;
    bool hsync_active_low;
    bool vsync_active_low;
    bool de_active_high;
    bool pclk_rising_edge;
};

static constexpr RgbPanelTiming DEFAULT_RGB_TIMING {
    800, 480,
    40, 48, 88,
    13, 3, 32,
    25000000UL,
    true, true,
    true,
    true
};

inline uint32_t calc_refresh_hz(const RgbPanelTiming &t) {
    uint32_t total_px_line = t.width + t.hsync_front_porch + t.hsync_pulse_width + t.hsync_back_porch;
    uint32_t total_lines = t.height + t.vsync_front_porch + t.vsync_pulse_width + t.vsync_back_porch;
    if (!total_px_line || !total_lines) return 0;
    return (uint32_t)((uint64_t)t.pclk_hz / ((uint64_t)total_px_line * total_lines));
}
