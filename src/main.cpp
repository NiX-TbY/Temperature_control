#include <Arduino.h>
#include <lvgl.h>
#include <ESP_Panel_Library.h>
#include <ESP_IOExpander.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"
#include "hmi_manager.h"
#include "io_controller.h"

// Global system state
SystemState g_system_state;

// Hardware instances
esp_panel::drivers::LCD_ST7262 *display_panel = nullptr;
esp_expander::CH422G *io_expander = nullptr;
RelayController *relay_controller = nullptr;
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

// LVGL Buffers (allocated in PSRAM)
#define LVGL_DRAW_BUF_SIZE (800 * 80)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;
static lv_disp_t *lvgl_display = nullptr;

// Timing variables
static unsigned long last_sensor_read = 0;
static unsigned long last_control_cycle = 0;
static unsigned long alarm_silence_start = 0;
static bool alarm_silenced = false;

// LVGL display flush callback
void display_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    if (display_panel) {
        display_panel->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, (uint16_t*)color_p);
    }
    lv_disp_flush_ready(disp_drv);
}

// LVGL touch read callback (placeholder)
void touchpad_read_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    // Full implementation would read from GT911 driver
    data->state = LV_INDEV_STATE_REL;
    data->point.x = 0;
    data->point.y = 0;
}

// Custom LVGL tick function
void lv_tick_task(void) {
    lv_tick_inc(5);
}

// FreeRTOS Tasks
void lvgl_task(void *pvParameter) {
    Serial.println("LVGL Task started on Core 0");
    TickType_t last_update = xTaskGetTickCount();
    
    while (1) {
        lv_timer_handler();
        
        // Update HMI with latest data every 250ms
        if (xTaskGetTickCount() - last_update > pdMS_TO_TICKS(250)) {
            hmi_update();
            last_update = xTaskGetTickCount();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void sensor_task(void *pvParameter) {
    Serial.println("Sensor Task started on Core 1");
    
    while (1) {
        // Read temperature sensors every 2 seconds
        sensors.requestTemperatures();
        delay(750); // Wait for conversion
        
        xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
        
        // Read all four sensors
        for (int i = 0; i < 4; i++) {
            float temp = sensors.getTempCByIndex(i);
            if (temp != DEVICE_DISCONNECTED_C) {
                g_system_state.sensor_values[i] = temp;
            } else {
                g_system_state.sensor_values[i] = -999.0; // Error value
                Serial.printf("Sensor %d disconnected\n", i);
            }
        }
        
        // Update main temperature (cabin sensor = index 0)
        if (g_system_state.sensor_values[0] > -999.0) {
            g_system_state.actual_temp_celsius = g_system_state.sensor_values[0];
        }
        
        xSemaphoreGive(g_system_state.mutex);
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 second cycle
    }
}

void control_logic_task(void *pvParameter) {
    Serial.println("Control Logic Task started on Core 1");
    
    while (1) {
        xSemaphoreTake(g_system_state.mutex, portMAX_DELAY);
        
        // === FAULT DETECTION ===
        g_system_state.active_fault_code = FaultCode::NONE;
        
        // Check IO expander communication
        if (!relay_controller->isConnected()) {
            g_system_state.active_fault_code = FaultCode::IO_EXPANDER_FAIL;
            hmi_update_fault_display("IO EXPANDER FAIL");
            // Shut down all outputs for safety
            g_system_state.relay_states = 0;
        }
        // Check cabin sensor
        else if (g_system_state.sensor_values[0] <= -999.0) {
            g_system_state.active_fault_code = FaultCode::CABIN_SENSOR_OPEN;
            hmi_update_fault_display("CABIN SENSOR FAIL");
        }
        // Check for high temperature alarm
        else if (g_system_state.actual_temp_celsius > 
                (g_system_state.setpoint_temp_celsius + HIGH_TEMP_ALARM_DIFFERENTIAL_C)) {
            g_system_state.active_fault_code = FaultCode::HIGH_TEMP_ALARM;
            g_system_state.alarm_status = AlarmState::HIGH_TEMP_ACTIVE;
            hmi_trigger_alarm_animation(true);
            
            // Sound buzzer unless silenced
            if (!alarm_silenced) {
                relay_controller->setBuzzer(true);
            }
        } else {
            // Clear fault display if no faults
            hmi_update_fault_display("");
            
            // Clear alarm if temperature is back to normal
            if (g_system_state.alarm_status != AlarmState::NONE) {
                g_system_state.alarm_status = AlarmState::NONE;
                hmi_trigger_alarm_animation(false);
                relay_controller->setBuzzer(false);
                alarm_silenced = false;
            }
        }
        
        // === CONTROL LOGIC ===
        if (g_system_state.active_fault_code == FaultCode::NONE ||
            g_system_state.active_fault_code == FaultCode::HIGH_TEMP_ALARM) {
            
            // Simple hysteresis control
            float temp_error = g_system_state.actual_temp_celsius - g_system_state.setpoint_temp_celsius;
            
            if (temp_error > TEMP_HYSTERESIS_C) {
                // Too warm - turn on cooling
                relay_controller->setCompressorState(true);
                relay_controller->setFanState(true);
                g_system_state.relay_states |= (1 << RELAY_PIN_COMPRESSOR);
                g_system_state.relay_states |= (1 << RELAY_PIN_EVAP_FAN);
            } else if (temp_error < -TEMP_HYSTERESIS_C) {
                // Cold enough - turn off cooling
                relay_controller->setCompressorState(false);
                relay_controller->setFanState(false);
                g_system_state.relay_states &= ~(1 << RELAY_PIN_COMPRESSOR);
                g_system_state.relay_states &= ~(1 << RELAY_PIN_EVAP_FAN);
            }
            
            // Handle defrost cycle
            if (g_system_state.defrost_active) {
                // Turn off compressor during defrost
                relay_controller->setCompressorState(false);
                relay_controller->setDefrostState(true);
                hmi_trigger_defrost_animation(true);
                
                // Simple timer-based defrost (in real implementation, use temperature termination)
                static unsigned long defrost_start = 0;
                if (defrost_start == 0) {
                    defrost_start = millis();
                }
                
                if (millis() - defrost_start > (DEFROST_DURATION_MIN * 60000)) {
                    // End defrost cycle
                    g_system_state.defrost_active = false;
                    relay_controller->setDefrostState(false);
                    hmi_trigger_defrost_animation(false);
                    defrost_start = 0;
                    Serial.println("Defrost cycle completed");
                }
            }
        } else {
            // Fault condition - shut down all outputs
            relay_controller->setCompressorState(false);
            relay_controller->setFanState(false);
            relay_controller->setDefrostState(false);
            g_system_state.relay_states = 0;
        }
        
        // Handle alarm silence timeout
        if (alarm_silenced && g_system_state.alarm_status == AlarmState::HIGH_TEMP_SILENCED) {
            if (millis() - alarm_silence_start > (ALARM_SILENCE_DURATION_MIN * 60000)) {
                // Re-enable alarm after silence period
                alarm_silenced = false;
                if (g_system_state.active_fault_code == FaultCode::HIGH_TEMP_ALARM) {
                    g_system_state.alarm_status = AlarmState::HIGH_TEMP_ACTIVE;
                    hmi_trigger_alarm_animation(true);
                }
            }
        }
        
        xSemaphoreGive(g_system_state.mutex);
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second control cycle
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("===========================================");
    Serial.println("Commercial Freezer Controller Booting...");
    Serial.println("ESP32-S3-Touch-LCD-4.3B Platform");
    Serial.println("===========================================");

    // Initialize global state
    g_system_state.mutex = xSemaphoreCreateMutex();
    g_system_state.setpoint_temp_celsius = DEFAULT_SETPOINT_C;
    g_system_state.actual_temp_celsius = 0.0;
    g_system_state.alarm_status = AlarmState::NONE;
    g_system_state.defrost_active = false;
    g_system_state.active_fault_code = FaultCode::NONE;
    g_system_state.relay_states = 0;
    
    // Initialize all sensor values to invalid
    for (int i = 0; i < 4; i++) {
        g_system_state.sensor_values[i] = -999.0;
    }

    Serial.println("1. Initializing I2C Bus...");
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("   I2C Bus initialized successfully");

    // === MANDATORY INITIALIZATION SEQUENCE ===
    Serial.println("2. Initializing CH422G I/O Expander...");
    io_expander = new esp_expander::CH422G(I2C_SCL_PIN, I2C_SDA_PIN, IOEXP_I2C_ADDR);
    if (!io_expander->init() || !io_expander->begin()) {
        Serial.println("   ERROR: Failed to initialize CH422G!");
        while(1) { delay(1000); } // Halt on critical error
    }
    Serial.println("   CH422G initialized successfully");

    Serial.println("3. Performing Display Hardware Reset...");
    io_expander->pinMode(CH422G_RST_PIN, OUTPUT);
    io_expander->digitalWrite(CH422G_RST_PIN, LOW);
    delay(20);
    io_expander->digitalWrite(CH422G_RST_PIN, HIGH);
    delay(50);
    Serial.println("   Display reset sequence completed");

    Serial.println("4. Initializing Display Panel (ST7262)...");
    esp_panel::drivers::BusRGB::Config bus_config;
    esp_panel::drivers::LCD::Config lcd_config;
    display_panel = new esp_panel::drivers::LCD_ST7262(bus_config, lcd_config);
    if (!display_panel->init() || !display_panel->begin()) {
        Serial.println("   ERROR: Failed to initialize display panel!");
    } else {
        Serial.println("   Display panel initialized successfully");
    }

    Serial.println("5. Enabling Display Backlight...");
    io_expander->pinMode(CH422G_BL_EN_PIN, OUTPUT);
    io_expander->digitalWrite(CH422G_BL_EN_PIN, HIGH);
    Serial.println("   Backlight enabled");

    Serial.println("6. Initializing LVGL Graphics Library...");
    lv_init();
    
    // Allocate display buffers in PSRAM
    buf1 = (lv_color_t*)heap_caps_malloc(LVGL_DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!buf1) {
        Serial.println("   ERROR: Failed to allocate LVGL buffer in PSRAM!");
        buf1 = (lv_color_t*)malloc(LVGL_DRAW_BUF_SIZE * sizeof(lv_color_t));
        Serial.println("   WARNING: Using internal RAM for LVGL buffer");
    }
    
    lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, LVGL_DRAW_BUF_SIZE);

    // Initialize display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 800;
    disp_drv.ver_res = 480;
    disp_drv.flush_cb = display_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lvgl_display = lv_disp_drv_register(&disp_drv);

    // Initialize input driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read_cb;
    lv_indev_drv_register(&indev_drv);
    
    Serial.println("   LVGL initialized successfully");

    Serial.println("7. Initializing Relay Controller (PCF8574)...");
    relay_controller = new RelayController(PCF8574_I2C_ADDR);
    if (!relay_controller->begin()) {
        Serial.println("   WARNING: Relay controller not found - operating in simulation mode");
    } else {
        Serial.println("   Relay controller initialized successfully");
    }

    Serial.println("8. Initializing Temperature Sensors...");
    sensors.begin();
    int sensor_count = sensors.getDeviceCount();
    Serial.printf("   Found %d DS18B20 sensors\n", sensor_count);
    if (sensor_count == 0) {
        Serial.println("   WARNING: No temperature sensors found");
    }

    Serial.println("9. Creating User Interface...");
    hmi_init();
    Serial.println("   HMI initialized successfully");

    Serial.println("10. Starting FreeRTOS Tasks...");
    
    // Create LVGL task on Core 0 (high priority for UI responsiveness)
    xTaskCreatePinnedToCore(
        lvgl_task,          // Task function
        "lvgl_task",        // Task name
        8192,               // Stack size
        nullptr,            // Parameters
        5,                  // Priority (high)
        nullptr,            // Task handle
        0                   // Core 0
    );

    // Create sensor task on Core 1
    xTaskCreatePinnedToCore(
        sensor_task,        // Task function
        "sensor_task",      // Task name
        4096,               // Stack size
        nullptr,            // Parameters
        2,                  // Priority (low)
        nullptr,            // Task handle
        1                   // Core 1
    );

    // Create control logic task on Core 1
    xTaskCreatePinnedToCore(
        control_logic_task, // Task function
        "control_task",     // Task name
        6144,               // Stack size
        nullptr,            // Parameters
        3,                  // Priority (medium)
        nullptr,            // Task handle
        1                   // Core 1
    );

    Serial.println("    All tasks created successfully");
    Serial.println();
    Serial.println("===========================================");
    Serial.println("SYSTEM READY - Commercial Freezer Controller");
    Serial.println("Hardware: ESP32-S3-Touch-LCD-4.3B");
    Serial.println("Firmware: Temperature Control v1.0");
    Serial.println("===========================================");
}

void loop() {
    // Main loop is not used; all processing is handled by FreeRTOS tasks
    // Provide LVGL tick
    static unsigned long last_tick = 0;
    if (millis() - last_tick > 5) {
        lv_tick_inc(5);
        last_tick = millis();
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
}