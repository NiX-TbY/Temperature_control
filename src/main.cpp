#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

// Local includes
#include "config.h"
#include "ESP32_Display_Panel.h"
#include "ESP_IOExpander.h"
#include "hmi_manager.h"
#include "sensor_manager.h"
#include "relay_controller.h"
#include "control_logic.h"

// Global objects
ESP_IOExpander* io_expander = nullptr;
esp_panel::drivers::LCD_ST7262* display_panel = nullptr;
lv_disp_t* lvgl_display = nullptr;

// Global state
GlobalState g_state;

// LVGL buffers (allocated in PSRAM)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t* buf1 = nullptr;
static lv_color_t* buf2 = nullptr;

// Task handles
TaskHandle_t lvgl_task_handle = nullptr;
TaskHandle_t control_task_handle = nullptr;
TaskHandle_t sensor_task_handle = nullptr;

// Timer callbacks
void alarm_silence_timer_callback(TimerHandle_t timer) {
    DEBUG_PRINTLN("Alarm silence period expired, re-enabling alarm");
    
    // Check if alarm condition still exists
    float cabin_temp, setpoint, alarm_diff;
    SAFE_STATE_READ(sensors.cabin_temp_c, cabin_temp);
    SAFE_STATE_READ(settings.setpoint_temp_c, setpoint);
    SAFE_STATE_READ(settings.alarm_differential_c, alarm_diff);
    
    if (cabin_temp > setpoint + alarm_diff) {
        SAFE_STATE_WRITE(status.alarm_state, AlarmState::HIGH_TEMP_ACTIVE);
        RelayController::getInstance().setBuzzer(true);
    }
}

// LVGL display flush callback
void display_flush_cb(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    if (display_panel) {
        int32_t width = area->x2 - area->x1 + 1;
        int32_t height = area->y2 - area->y1 + 1;
        display_panel->drawBitmap(area->x1, area->y1, width, height, (const uint16_t*)color_p);
    }
    lv_disp_flush_ready(disp_drv);
}

// LVGL touch read callback (placeholder for GT911 implementation)
void touchpad_read_cb(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    // TODO: Implement GT911 touch controller reading
    // For now, set to released state
    data->state = LV_INDEV_STATE_REL;
    data->point.x = 0;
    data->point.y = 0;
}

// FreeRTOS Tasks
void lvgl_task(void* pvParameter) {
    DEBUG_PRINTLN("LVGL task started on core " + String(xPortGetCoreID()));
    
    TickType_t last_tick = xTaskGetTickCount();
    
    while (1) {
        // Process LVGL timer handler
        lv_timer_handler();
        
        // Update HMI
        HMIManager::getInstance().update();
        
        // Maintain consistent timing
        vTaskDelayUntil(&last_tick, pdMS_TO_TICKS(LVGL_TICK_INTERVAL_MS));
    }
}

void sensor_task(void* pvParameter) {
    DEBUG_PRINTLN("Sensor task started on core " + String(xPortGetCoreID()));
    
    TickType_t last_wake = xTaskGetTickCount();
    
    while (1) {
        // Read all temperature sensors
        SensorManager::getInstance().readAllSensors();
        
        // Wait for next reading
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

void control_task(void* pvParameter) {
    DEBUG_PRINTLN("Control task started on core " + String(xPortGetCoreID()));
    
    TickType_t last_wake = xTaskGetTickCount();
    
    while (1) {
        // Run main control algorithm
        ControlLogic::getInstance().runControlLoop();
        
        // Wait for next control cycle
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(CONTROL_LOOP_INTERVAL_MS));
    }
}

bool initHardware() {
    DEBUG_PRINTLN("Initializing hardware...");
    
    // Initialize I2C bus
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQUENCY);
    Wire.setTimeout(1000);  // 1 second timeout
    
    // Initialize CH422G I/O expander (mandatory first step)
    io_expander = new ESP_IOExpander(CH422G_I2C_ADDR);
    if (!io_expander->init()) {
        DEBUG_PRINTLN("ERROR: Failed to initialize CH422G I/O expander");
        return false;
    }
    
    if (!io_expander->begin()) {
        DEBUG_PRINTLN("ERROR: Failed to begin CH422G I/O expander");
        return false;
    }
    
    DEBUG_PRINTLN("CH422G I/O expander initialized successfully");
    
    // Configure display RGB interface
    esp_panel::drivers::RGB_Config rgb_config = {
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_HEIGHT,
        .de_pin = DISPLAY_DE_PIN,
        .vsync_pin = DISPLAY_VSYNC_PIN,
        .hsync_pin = DISPLAY_HSYNC_PIN,
        .pclk_pin = DISPLAY_PCLK_PIN,
        .data_pins = {
            DISPLAY_DATA0_PIN, DISPLAY_DATA1_PIN, DISPLAY_DATA2_PIN, DISPLAY_DATA3_PIN,
            DISPLAY_DATA4_PIN, DISPLAY_DATA5_PIN, DISPLAY_DATA6_PIN, DISPLAY_DATA7_PIN,
            DISPLAY_DATA8_PIN, DISPLAY_DATA9_PIN, DISPLAY_DATA10_PIN, DISPLAY_DATA11_PIN,
            DISPLAY_DATA12_PIN, DISPLAY_DATA13_PIN, DISPLAY_DATA14_PIN, DISPLAY_DATA15_PIN,
            DISPLAY_DATA16_PIN, DISPLAY_DATA17_PIN, DISPLAY_DATA18_PIN, DISPLAY_DATA19_PIN,
            DISPLAY_DATA20_PIN, DISPLAY_DATA21_PIN, DISPLAY_DATA22_PIN, DISPLAY_DATA23_PIN
        },
        .pclk_freq_hz = 25000000,  // 25 MHz
        .hsync_front_porch = 8,
        .hsync_back_porch = 8,
        .hsync_pulse_width = 4,
        .vsync_front_porch = 8,
        .vsync_back_porch = 8,
        .vsync_pulse_width = 4,
        .hsync_idle_low = false,
        .vsync_idle_low = false,
        .de_idle_high = false,
        .pclk_active_neg = false
    };
    
    // Initialize display panel
    display_panel = new esp_panel::drivers::LCD_ST7262(rgb_config, io_expander);
    if (!display_panel->init()) {
        DEBUG_PRINTLN("ERROR: Failed to initialize display panel");
        return false;
    }
    
    if (!display_panel->begin()) {
        DEBUG_PRINTLN("ERROR: Failed to begin display panel");
        return false;
    }
    
    // Enable backlight
    display_panel->enable();
    
    DEBUG_PRINTLN("Display panel initialized successfully");
    return true;
}

bool initLVGL() {
    DEBUG_PRINTLN("Initializing LVGL...");
    
    // Initialize LVGL
    lv_init();
    
    // Allocate display buffers in PSRAM
    buf1 = (lv_color_t*)heap_caps_malloc(LVGL_BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!buf1) {
        DEBUG_PRINTLN("ERROR: Failed to allocate LVGL buffer 1 in PSRAM");
        return false;
    }
    
    buf2 = (lv_color_t*)heap_caps_malloc(LVGL_BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!buf2) {
        DEBUG_PRINTLN("ERROR: Failed to allocate LVGL buffer 2 in PSRAM");
        heap_caps_free(buf1);
        return false;
    }
    
    DEBUG_PRINTF("LVGL buffers allocated in PSRAM: %p, %p\n", buf1, buf2);
    
    // Initialize display buffer
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LVGL_BUFFER_SIZE);
    
    // Initialize display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DISPLAY_WIDTH;
    disp_drv.ver_res = DISPLAY_HEIGHT;
    disp_drv.flush_cb = display_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lvgl_display = lv_disp_drv_register(&disp_drv);
    
    // Initialize touch input driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read_cb;
    lv_indev_drv_register(&indev_drv);
    
    DEBUG_PRINTLN("LVGL initialized successfully");
    return true;
}

bool initGlobalState() {
    DEBUG_PRINTLN("Initializing global state...");
    
    // Create mutex
    g_state.mutex = xSemaphoreCreateMutex();
    if (!g_state.mutex) {
        DEBUG_PRINTLN("ERROR: Failed to create global state mutex");
        return false;
    }
    
    // Initialize default settings
    g_state.settings.setpoint_temp_c = DEFAULT_SETPOINT_C;
    g_state.settings.hysteresis_c = TEMP_HYSTERESIS_C;
    g_state.settings.alarm_differential_c = TEMP_ALARM_DIFFERENTIAL_C;
    g_state.settings.defrost_interval_hours = DEFROST_INTERVAL_HOURS;
    g_state.settings.defrost_duration_minutes = DEFROST_DURATION_MS / 60000;
    g_state.settings.defrost_termination_temp_c = DEFROST_TERMINATION_TEMP_C;
    g_state.settings.defrost_type = DefrostType::HOT_GAS;
    g_state.settings.enable_adaptive_defrost = true;
    
    // Initialize sensor data
    for (int i = 0; i < 4; i++) {
        g_state.sensors.sensor_valid[i] = false;
    }
    g_state.sensors.cabin_temp_c = DEFAULT_SETPOINT_C;
    g_state.sensors.evap_temp_c = DEFAULT_SETPOINT_C - 10.0f;
    g_state.sensors.condenser_temp_c = 25.0f;
    g_state.sensors.suction_temp_c = DEFAULT_SETPOINT_C - 5.0f;
    g_state.sensors.last_read_ms = 0;
    
    // Initialize system status
    g_state.status.current_state = SystemState::STARTUP;
    g_state.status.alarm_state = AlarmState::NONE;
    g_state.status.active_fault = FaultCode::NONE;
    g_state.status.compressor_running = false;
    g_state.status.evap_fan_running = false;
    g_state.status.defrost_active = false;
    g_state.status.manual_defrost_requested = false;
    g_state.status.defrost_start_time = 0;
    g_state.status.last_defrost_time = 0;
    g_state.status.compressor_runtime_hours = 0;
    g_state.status.defrost_count_today = 0;
    
    // Create timers
    g_state.alarm_silence_timer = xTimerCreate(
        "AlarmSilenceTimer",
        pdMS_TO_TICKS(ALARM_SILENCE_DURATION_MS),
        pdFALSE,  // One-shot timer
        nullptr,
        alarm_silence_timer_callback
    );
    
    g_state.service_menu_active = false;
    g_state.hmi_needs_update = true;
    
    DEBUG_PRINTLN("Global state initialized successfully");
    return true;
}

bool initManagers() {
    DEBUG_PRINTLN("Initializing managers...");
    
    // Initialize sensor manager
    if (!SensorManager::getInstance().init()) {
        DEBUG_PRINTLN("ERROR: Failed to initialize sensor manager");
        return false;
    }
    
    // Initialize relay controller
    if (!RelayController::getInstance().init()) {
        DEBUG_PRINTLN("ERROR: Failed to initialize relay controller");
        return false;
    }
    
    // Initialize control logic
    if (!ControlLogic::getInstance().init()) {
        DEBUG_PRINTLN("ERROR: Failed to initialize control logic");
        return false;
    }
    
    // Initialize HMI manager
    if (!HMIManager::getInstance().init()) {
        DEBUG_PRINTLN("ERROR: Failed to initialize HMI manager");
        return false;
    }
    
    DEBUG_PRINTLN("All managers initialized successfully");
    return true;
}

void createTasks() {
    DEBUG_PRINTLN("Creating FreeRTOS tasks...");
    
    // Create LVGL task on Core 0 (high priority for smooth UI)
    xTaskCreatePinnedToCore(
        lvgl_task,
        "LVGL Task",
        TASK_STACK_SIZE_LVGL,
        nullptr,
        TASK_PRIORITY_LVGL,
        &lvgl_task_handle,
        0  // Core 0
    );
    
    // Create sensor task on Core 1 (lower priority)
    xTaskCreatePinnedToCore(
        sensor_task,
        "Sensor Task",
        TASK_STACK_SIZE_SENSORS,
        nullptr,
        TASK_PRIORITY_SENSORS,
        &sensor_task_handle,
        1  // Core 1
    );
    
    // Create control task on Core 1 (medium priority)
    xTaskCreatePinnedToCore(
        control_task,
        "Control Task",
        TASK_STACK_SIZE_CONTROL,
        nullptr,
        TASK_PRIORITY_CONTROL,
        &control_task_handle,
        1  // Core 1
    );
    
    DEBUG_PRINTLN("All tasks created successfully");
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        // Wait for serial or timeout
    }
    
    DEBUG_PRINTLN("\n\n=== ESP32-S3 Commercial Freezer Controller ===");
    DEBUG_PRINTLN("Firmware Version: 1.0.0");
    DEBUG_PRINTF("Free Heap: %d bytes\n", ESP.getFreeHeap());
    DEBUG_PRINTF("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    
    // Initialize hardware in mandatory sequence
    if (!initHardware()) {
        DEBUG_PRINTLN("FATAL: Hardware initialization failed");
        while (1) delay(1000);
    }
    
    // Initialize LVGL graphics library
    if (!initLVGL()) {
        DEBUG_PRINTLN("FATAL: LVGL initialization failed");
        while (1) delay(1000);
    }
    
    // Initialize global state
    if (!initGlobalState()) {
        DEBUG_PRINTLN("FATAL: Global state initialization failed");
        while (1) delay(1000);
    }
    
    // Initialize all managers
    if (!initManagers()) {
        DEBUG_PRINTLN("FATAL: Manager initialization failed");
        while (1) delay(1000);
    }
    
    // Create FreeRTOS tasks
    createTasks();
    
    DEBUG_PRINTLN("=== System initialization complete ===");
    DEBUG_PRINTF("Total heap used: %d bytes\n", ESP.getHeapSize() - ESP.getFreeHeap());
    DEBUG_PRINTF("PSRAM used: %d bytes\n", ESP.getPsramSize() - ESP.getFreePsram());
    
    // System is now running via FreeRTOS tasks
}

void loop() {
    // Main loop is not used - all processing handled by FreeRTOS tasks
    // This prevents watchdog triggers
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Optional: Add watchdog feeding or system health monitoring here
    static uint32_t last_status_print = 0;
    if (millis() - last_status_print > 30000) {  // Every 30 seconds
        DEBUG_PRINTF("System uptime: %lu seconds, Free heap: %d\n", 
                     millis() / 1000, ESP.getFreeHeap());
        last_status_print = millis();
    }
}