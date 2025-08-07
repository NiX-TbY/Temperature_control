#pragma once

// LVGL Configuration for ESP32-S3 Temperature Controller

// Color settings
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   0

// Memory settings
#define LV_MEM_CUSTOM      1
#define LV_MEM_SIZE        (64U * 1024U)  // 64KB for LVGL heap

// Display settings
#define LV_DISP_DEF_REFR_PERIOD    5    // Refresh period in ms (200 Hz)
#define LV_INDEV_DEF_READ_PERIOD   5    // Input device read period in ms

// Feature settings
#define LV_USE_PERF_MONITOR        1    // Show performance monitor
#define LV_USE_MEM_MONITOR         1    // Show memory monitor

// Font settings
#define LV_FONT_MONTSERRAT_12      1
#define LV_FONT_MONTSERRAT_14      1
#define LV_FONT_MONTSERRAT_16      1
#define LV_FONT_MONTSERRAT_18      1
#define LV_FONT_MONTSERRAT_20      1
#define LV_FONT_MONTSERRAT_24      1
#define LV_FONT_MONTSERRAT_28      1
#define LV_FONT_MONTSERRAT_32      1
#define LV_FONT_MONTSERRAT_36      1
#define LV_FONT_MONTSERRAT_40      1
#define LV_FONT_MONTSERRAT_44      1
#define LV_FONT_MONTSERRAT_48      1

// Widget settings
#define LV_USE_ANIMIMG             1
#define LV_USE_BTN                 1
#define LV_USE_LABEL               1
#define LV_USE_SLIDER              1
#define LV_USE_TABVIEW             1
#define LV_USE_CHART               1

// Animation settings
#define LV_USE_ANIMATION           1

// Tick settings
#define LV_TICK_CUSTOM             1

// Logging
#define LV_USE_LOG                 1
#define LV_LOG_LEVEL               LV_LOG_LEVEL_INFO

// Demo and examples
#define LV_BUILD_EXAMPLES          0