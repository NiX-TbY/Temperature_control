#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "config.h"
#include "esp_panel.h"
#include "ESP_IOExpander.h"
#include "relay_controller.h"
#include "temperature_sensors.h"

// Global system state
SystemState g_system_state;

// Hardware components
esp_expander::CH422G* io_expander = nullptr;
esp_panel::drivers::LCD_ST7262* display_panel = nullptr;

// LVGL display buffer
#define LVGL_DRAW_BUF_SIZE (800 * 80)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t* buf1 = nullptr;

// LVGL objects
static lv_obj_t* label_setpoint;
static lv_obj_t* label_actual_temp;
static lv_obj_t* btn_up;
static lv_obj_t* btn_down;
static lv_obj_t* btn_defrost;
static lv_obj_t* label_status;

// Custom LVGL memory management using PSRAM
#if LV_MEM_CUSTOM
void* lv_mem_alloc(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
}

void lv_mem_free(void* ptr) {
    heap_caps_free(ptr);
}

void* lv_mem_realloc(void* ptr, size_t new_size) {
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
}
#endif

// LVGL display flush callback
void display_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    if (display_panel) {
        display_panel->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, (uint16_t*)color_p);
    }
    lv_disp_flush_ready(disp_drv);
}

// LVGL input device callback (placeholder)
void touchpad_read_cb(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    // TODO: Implement GT911 touch reading
    data->state = LV_INDEV_STATE_REL;
}

// Event handlers
void btn_up_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
        g_system_state.setpoint_temp_celsius += 1.0f;
        if (g_system_state.setpoint_temp_celsius > 0.0f) {
            g_system_state.setpoint_temp_celsius = 0.0f;
        }
        xSemaphoreGive(g_system_state.mutex);
        Serial.println("Setpoint increased");
    }
}

void btn_down_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
        g_system_state.setpoint_temp_celsius -= 1.0f;
        if (g_system_state.setpoint_temp_celsius < -30.0f) {
            g_system_state.setpoint_temp_celsius = -30.0f;
        }
        xSemaphoreGive(g_system_state.mutex);
        Serial.println("Setpoint decreased");
    }
}

void btn_defrost_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) {
        xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
        g_system_state.defrost_active = !g_system_state.defrost_active;
        xSemaphoreGive(g_system_state.mutex);
        Serial.println("Manual defrost toggled");
    }
}

// Create the user interface
void create_ui() {
    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // Left column container
    lv_obj_t* left_col = lv_obj_create(scr);
    lv_obj_set_size(left_col, 200, 480);
    lv_obj_align(left_col, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(left_col, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_border_width(left_col, 2, 0);
    lv_obj_set_style_border_color(left_col, lv_color_hex(0x333333), 0);
    lv_obj_set_flex_flow(left_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left_col, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Setpoint display
    label_setpoint = lv_label_create(left_col);
    lv_obj_set_style_text_font(label_setpoint, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label_setpoint, lv_color_hex(0x168AEE), 0);
    lv_label_set_text(label_setpoint, "-18°C");
    
    // Up button
    btn_up = lv_btn_create(left_col);
    lv_obj_set_size(btn_up, 150, 80);
    lv_obj_set_style_bg_color(btn_up, lv_color_hex(0x003366), 0);
    lv_obj_add_event_cb(btn_up, btn_up_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* label_up = lv_label_create(btn_up);
    lv_label_set_text(label_up, LV_SYMBOL_UP);
    lv_obj_set_style_text_color(label_up, lv_color_white(), 0);
    lv_obj_center(label_up);

    // Down button
    btn_down = lv_btn_create(left_col);
    lv_obj_set_size(btn_down, 150, 80);
    lv_obj_set_style_bg_color(btn_down, lv_color_hex(0x003366), 0);
    lv_obj_add_event_cb(btn_down, btn_down_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* label_down = lv_label_create(btn_down);
    lv_label_set_text(label_down, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_color(label_down, lv_color_white(), 0);
    lv_obj_center(label_down);

    // Defrost button
    btn_defrost = lv_btn_create(left_col);
    lv_obj_set_size(btn_defrost, 150, 80);
    lv_obj_set_style_bg_color(btn_defrost, lv_color_hex(0xAD8E6E), 0);
    lv_obj_add_event_cb(btn_defrost, btn_defrost_event_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_t* label_defrost = lv_label_create(btn_defrost);
    lv_label_set_text(label_defrost, "DEFROST");
    lv_obj_set_style_text_color(label_defrost, lv_color_white(), 0);
    lv_obj_center(label_defrost);

    // Right column - actual temperature
    lv_obj_t* right_col = lv_obj_create(scr);
    lv_obj_set_size(right_col, 550, 480);
    lv_obj_align(right_col, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(right_col, lv_color_black(), 0);
    lv_obj_set_style_border_width(right_col, 0, 0);

    label_actual_temp = lv_label_create(right_col);
    lv_obj_set_style_text_font(label_actual_temp, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(label_actual_temp, lv_color_white(), 0);
    lv_label_set_text(label_actual_temp, "-18.2°C");
    lv_obj_center(label_actual_temp);

    // Status label
    label_status = lv_label_create(scr);
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_text_color(label_status, lv_color_hex(0x00FF00), 0);
    lv_label_set_text(label_status, "SYSTEM READY");
}

// Update the display with current values
void update_display() {
    static unsigned long last_update = 0;
    unsigned long now = millis();
    
    if (now - last_update < 250) return; // Update at 4Hz
    last_update = now;

    xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
    
    // Update setpoint
    lv_label_set_text_fmt(label_setpoint, "%.0f°C", g_system_state.setpoint_temp_celsius);
    
    // Update actual temperature
    lv_label_set_text_fmt(label_actual_temp, "%.1f°C", g_system_state.actual_temp_celsius);
    
    // Update status based on system state
    if (g_system_state.active_fault_code != FaultCode::NONE) {
        lv_obj_set_style_text_color(label_status, lv_color_hex(0xFF0000), 0);
        lv_label_set_text(label_status, "FAULT DETECTED");
    } else if (g_system_state.defrost_active) {
        lv_obj_set_style_text_color(label_status, lv_color_hex(0xFFFF00), 0);
        lv_label_set_text(label_status, "DEFROST ACTIVE");
    } else if (g_system_state.alarm_status != AlarmState::NONE) {
        lv_obj_set_style_text_color(label_status, lv_color_hex(0xFF8800), 0);
        lv_label_set_text(label_status, "HIGH TEMP ALARM");
    } else {
        lv_obj_set_style_text_color(label_status, lv_color_hex(0x00FF00), 0);
        lv_label_set_text(label_status, "SYSTEM READY");
    }
    
    xSemaphoreGive(g_system_state.mutex);
}

// FreeRTOS Tasks
void lvgl_task(void* parameter) {
    while (1) {
        lv_timer_handler();
        update_display();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void sensor_task(void* parameter) {
    while (1) {
        if (g_temp_sensor_manager) {
            g_temp_sensor_manager->updateTemperatures();
        }
        vTaskDelay(pdMS_TO_TICKS(2000)); // Update every 2 seconds
    }
}

void control_task(void* parameter) {
    while (1) {
        // Basic control logic
        xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
        
        float cabin_temp = g_system_state.actual_temp_celsius;
        float setpoint = g_system_state.setpoint_temp_celsius;
        
        // Simple hysteresis control
        if (cabin_temp > setpoint + TEMP_HYSTERESIS_C) {
            // Too warm, turn on compressor
            if (g_relay_controller) {
                g_relay_controller->setCompressorState(true);
                g_relay_controller->setFanState(true);
            }
        } else if (cabin_temp < setpoint - TEMP_HYSTERESIS_C) {
            // Too cold, turn off compressor
            if (g_relay_controller) {
                g_relay_controller->setCompressorState(false);
                g_relay_controller->setFanState(false);
            }
        }
        
        // Check for high temperature alarm
        if (cabin_temp > setpoint + HIGH_TEMP_ALARM_DIFFERENTIAL_C) {
            g_system_state.alarm_status = AlarmState::HIGH_TEMP_ACTIVE;
            if (g_relay_controller) {
                g_relay_controller->setBuzzer(true);
            }
        } else {
            g_system_state.alarm_status = AlarmState::NONE;
            if (g_relay_controller) {
                g_relay_controller->setBuzzer(false);
            }
        }
        
        xSemaphoreGive(g_system_state.mutex);
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Run control loop every second
    }
}

bool init_hardware() {
    Serial.println("Initializing hardware...");
    
    // Initialize I2C bus
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("I2C bus initialized");
    
    // Initialize CH422G I/O Expander
    io_expander = new esp_expander::CH422G(I2C_SCL_PIN, I2C_SDA_PIN, IOEXP_I2C_ADDR);
    if (!io_expander->init() || !io_expander->begin()) {
        Serial.println("Failed to initialize CH422G I/O expander");
        return false;
    }
    Serial.println("CH422G I/O expander initialized");
    
    // Hardware reset display via I/O expander
    io_expander->pinMode(CH422G_RST_PIN, OUTPUT);
    io_expander->digitalWrite(CH422G_RST_PIN, LOW);
    delay(20);
    io_expander->digitalWrite(CH422G_RST_PIN, HIGH);
    delay(50);
    Serial.println("Display reset completed");
    
    // Initialize display panel
    esp_panel::drivers::BusRGB::Config bus_config = {
        .de_pin = 5,
        .vsync_pin = 3,
        .hsync_pin = 46,
        .pclk_pin = 7,
        .data_pins = {14, 21, 47, 48, 45, 38, 39, 40, 0, 1, 2, 42, 41, 17, 18, 12, 13, 19, 20, 6, 10, 11, 16, 15}
    };
    
    esp_panel::drivers::LCD::Config lcd_config = {
        .width = 800,
        .height = 480,
        .bits_per_pixel = 16,
        .pixel_clock_hz = 25000000,
        .invert_color = false
    };
    
    display_panel = new esp_panel::drivers::LCD_ST7262(bus_config, lcd_config);
    if (!display_panel->init() || !display_panel->begin()) {
        Serial.println("Failed to initialize display panel");
        return false;
    }
    Serial.println("Display panel initialized");
    
    // Enable backlight
    io_expander->pinMode(CH422G_BL_EN_PIN, OUTPUT);
    io_expander->digitalWrite(CH422G_BL_EN_PIN, HIGH);
    Serial.println("Display backlight enabled");
    
    return true;
}

bool init_lvgl() {
    Serial.println("Initializing LVGL...");
    
    // Initialize LVGL
    lv_init();
    
    // Allocate draw buffer in PSRAM
    buf1 = (lv_color_t*)heap_caps_malloc(LVGL_DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!buf1) {
        Serial.println("Failed to allocate LVGL draw buffer");
        return false;
    }
    
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LVGL_DRAW_BUF_SIZE);
    
    // Initialize display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 800;
    disp_drv.ver_res = 480;
    disp_drv.flush_cb = display_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    // Initialize input driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read_cb;
    lv_indev_drv_register(&indev_drv);
    
    Serial.println("LVGL initialized");
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== Commercial Freezer Controller ===");
    Serial.println("Booting...");
    
    // Initialize global system state
    g_system_state.mutex = xSemaphoreCreateMutex();
    g_system_state.setpoint_temp_celsius = DEFAULT_SETPOINT_C;
    g_system_state.actual_temp_celsius = DEFAULT_SETPOINT_C;
    g_system_state.alarm_status = AlarmState::NONE;
    g_system_state.defrost_active = false;
    g_system_state.active_fault_code = FaultCode::NONE;
    g_system_state.relay_states = 0;
    
    for (int i = 0; i < 4; i++) {
        g_system_state.sensor_values[i] = -999.0f;
    }
    
    // Initialize hardware components
    if (!init_hardware()) {
        Serial.println("Hardware initialization failed!");
        while (1) delay(1000);
    }
    
    // Initialize LVGL
    if (!init_lvgl()) {
        Serial.println("LVGL initialization failed!");
        while (1) delay(1000);
    }
    
    // Create user interface
    create_ui();
    
    // Initialize peripheral managers
    g_relay_controller = new RelayController(PCF8574_I2C_ADDR);
    if (!g_relay_controller->begin()) {
        Serial.println("Warning: PCF8574 relay controller not found");
    } else {
        Serial.println("PCF8574 relay controller initialized");
    }
    
    g_temp_sensor_manager = new TemperatureSensorManager();
    if (!g_temp_sensor_manager->begin()) {
        Serial.println("Warning: Temperature sensors not found");
    } else {
        Serial.println("Temperature sensors initialized");
    }
    
    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(lvgl_task, "LVGL_Task", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(sensor_task, "Sensor_Task", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(control_task, "Control_Task", 4096, NULL, 3, NULL, 1);
    
    Serial.println("System initialization complete!");
    Serial.println("Core 0: LVGL UI Task");
    Serial.println("Core 1: Sensor and Control Tasks");
}

void loop() {
    // All processing handled by FreeRTOS tasks
    vTaskDelay(portMAX_DELAY);
}