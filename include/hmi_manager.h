#pragma once
#include <lvgl.h>

void hmi_init();
void hmi_update();

// Functions to be called by other tasks to trigger UI changes
void hmi_trigger_alarm_animation(bool active);
void hmi_set_alarm_acknowledged(bool acknowledged);
void hmi_trigger_defrost_animation(bool active);
void hmi_update_fault_display(const char* fault_text);