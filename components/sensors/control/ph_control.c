#include "ph_control.h"

#include <stdbool.h>
#include <esp_log.h>
#include <esp_err.h>
#include <string.h>

#include "rtc.h"
#include "ph_reading.h"
#include "control_task.h"
#include "sync_sensors.h"
#include "ports.h"
#include "control_settings_keys.h"
#include "ec_control.h"
#include "sensor.h"

struct sensor_control* get_ph_control() { return &ph_control; }

void check_ph() { // Check ph
	if(!control_get_active(get_ec_control())) {
		int result = control_check_sensor(&ph_control, sensor_get_value(get_ph_sensor()));
		if(result == -1) ph_up_pump();
		else if(result == 1) ph_down_pump();
	}
}

void ph_up_pump() {
	set_gpio_on(PH_UP_PUMP_GPIO);
	ESP_LOGI(PH_TAG, "pH up pump on");

	// Enable dose timer
	ESP_LOGI(PH_TAG, "%p", &dev);
	control_start_dose_timer(&ph_control);
}

void ph_down_pump() {
	set_gpio_on(PH_DOWN_PUMP_GPIO);
	ESP_LOGI(PH_TAG, "pH down pump on");

	// Enable dose timer
	control_start_dose_timer(&ph_control);
}

void ph_pump_off() {
	set_gpio_off(PH_UP_PUMP_GPIO);
	set_gpio_off(PH_DOWN_PUMP_GPIO);
	ESP_LOGI(PH_TAG, "pH pumps off");

	// Enable wait timer
	control_start_wait_timer(&ph_control);
}

void ph_update_settings(cJSON *item) {
	nvs_handle_t *handle = nvs_get_handle(PH_NAMESPACE);
	control_update_settings(&ph_control, item, handle);

	nvs_commit_data(handle);
	ESP_LOGI(PH_TAG, "Updated settings and committed data to NVS");
}

void ph_get_nvs_settings() {
	control_get_nvs_settings(&ph_control, PH_NAMESPACE);
	ESP_LOGI(PH_TAG ,"Updated settings from NVS");
}
