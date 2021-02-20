#include "grow_manager.h"

#include <esp_log.h>
#include <esp_err.h>
#include <FreeRTOS/freertos.h>
#include <freertos/task.h>

#include "nvs_manager.h"
#include "nvs_namespace_keys.h"
#include "ec_reading.h"
#include "ph_reading.h"
#include "water_temp_reading.h"
#include "sync_sensors.h"
#include "mqtt_manager.h"
#include "control_task.h"
#include "rf_transmitter.h"
#include "rtc.h"

void init_grow_manager() {
	uint8_t status; // Holds vars coming from NVS

	// Check for sensor settings status
	if(!nvs_get_data(&status, GROW_SETTINGS_NVS_NAMESPACE, SETTINGS_RECEIVED_KEY, UINT8) || !status) {
		is_settings_received = false;
		stop_grow_cycle();
		return;
	} else {
		is_settings_received = true;
	}

	// Check for grow cycle status
	if(!nvs_get_data(&status, GROW_SETTINGS_NVS_NAMESPACE, GROW_ACTIVE_KEY, UINT8) || !status) {
		stop_grow_cycle();
	} else {
		start_grow_cycle();
	}
}

void push_grow_status() {
	// Store in NVS
	struct NVS_Data *data = nvs_init_data();
	nvs_add_data(data, GROW_ACTIVE_KEY, UINT8, &is_grow_active);
}

void push_grow_settings_status() {
	// Store in NVS
	struct NVS_Data *data = nvs_init_data();
	nvs_add_data(data, GROW_ACTIVE_KEY, UINT8, &is_settings_received);
}

void suspend_tasks() {
	// Core 0 tasks
	vTaskSuspend(rf_transmitter_task_handle);
	vTaskSuspend(timer_alarm_task_handle);
	vTaskSuspend(publish_task_handle);
	vTaskSuspend(sensor_control_task_handle);

	// Core 1
	vTaskSuspend(*sensor_get_task_handle(get_water_temp_sensor()));
	vTaskSuspend(*sensor_get_task_handle(get_ec_sensor()));
	vTaskSuspend(*sensor_get_task_handle(get_ph_sensor()));
	vTaskSuspend(sync_task_handle);
}

void resume_tasks() {
	// Core 0 tasks
	vTaskResume(rf_transmitter_task_handle);
	vTaskResume(timer_alarm_task_handle);
	vTaskResume(publish_task_handle);
	vTaskResume(sensor_control_task_handle);

	// Core 1
	vTaskResume(*sensor_get_task_handle(get_water_temp_sensor()));
	vTaskResume(*sensor_get_task_handle(get_ec_sensor()));
	vTaskResume(*sensor_get_task_handle(get_ph_sensor()));
	vTaskResume(sync_task_handle);
}


void start_grow_cycle() {
	// Don't start unless settings have been received
	if(!is_settings_received){
		ESP_LOGE("Error", "Attempted to start grow cycle before sending settings");
		return;
	}

	// Set active to true and store in NVS
	is_grow_active = true;
	push_grow_status();

	resume_tasks();
	ESP_LOGI("", "Started Grow Cycle");
}

void stop_grow_cycle() {
	// Set active to false ands store in NVS
	is_grow_active = false;
	push_grow_status();

	ESP_LOGI("", "Stopped Grow Cycle");
	suspend_tasks();
}

void settings_received() {
	push_grow_settings_status();
	is_settings_received = true;
}

bool get_is_settings_received() { return is_settings_received; }
bool get_is_grow_active() { return is_grow_active; }
