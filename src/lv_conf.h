#pragma once

// LVGL Configuration for ESP32-S3 with PSRAM

// Color settings
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

// Memory settings
#define LV_MEM_CUSTOM 1
#define LV_MEM_SIZE (128 * 1024U)  // 128KB for LVGL heap

// GPU settings  
#define LV_USE_GPU_ARM2D 0

// Feature usage
#define LV_USE_ANIMATION 1
#define LV_USE_SHADOW 1
#define LV_USE_BLEND_MODES 1
#define LV_USE_OPA_SCALE 1
#define LV_USE_IMG_TRANSFORM 1

// Widget usage
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CANVAS 1
#define LV_USE_CHART 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_LIST 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1
#define LV_USE_TABVIEW 1

// Themes
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_BASIC 1

// Layouts
#define LV_USE_LAYOUT_FLEX 1
#define LV_USE_LAYOUT_GRID 1

// Others
#define LV_USE_FILESYSTEM 0
#define LV_USE_USER_DATA 1

// Asserts and logs
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

#define LV_USE_LOG 1
#if LV_USE_LOG
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1
#endif

// Performance settings
#define LV_DISP_DEF_REFR_PERIOD 16  // 60 FPS
#define LV_INDEV_DEF_READ_PERIOD 30 // 30ms touch read period

// Font usage
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_48 1

// Default font
#define LV_FONT_DEFAULT &lv_font_montserrat_14

// Custom memory management for PSRAM
#if LV_MEM_CUSTOM
void * lv_mem_alloc(size_t size);
void lv_mem_free(void * ptr);
void * lv_mem_realloc(void * ptr, size_t new_size);
#endif