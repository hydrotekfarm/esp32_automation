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
	ESP_LOGI("", "pH up pump on");

	// Enable dose timer
	ESP_LOGI("sdf", "%p", &dev);
	control_start_dose_timer(&ph_control);
}

void ph_down_pump() {
	set_gpio_on(PH_DOWN_PUMP_GPIO);
	ESP_LOGI("", "pH down pump on");

	// Enable dose timer
	control_start_dose_timer(&ph_control);
}

void ph_pump_off() {
	set_gpio_off(PH_UP_PUMP_GPIO);
	set_gpio_off(PH_DOWN_PUMP_GPIO);
	ESP_LOGI("", "pH pumps off");

	// Enable wait timer
	control_start_wait_timer(&ph_control);
}

void ph_update_settings(cJSON *item) {
	nvs_handle_t *handle = nvs_get_handle(EC_NAMESPACE);
	control_update_settings(&ph_control, item, handle);
	cJSON *element = item->child;
	while(element != NULL) {
		char *key = element->string;
		if(strcmp(key, CONTROL) == 0) {
			cJSON *control_element = element->child;
			while(control_element != NULL) {
				char *control_key = control_element->string;
				if(strcmp(control_key, PUMPS) == 0) {
					//TODO update target value
					cJSON *pumps_element = control_element->child;
					while(pumps_element != NULL) {
						char *pumps_key = pumps_element->string;
						if(strcmp(pumps_key, PUMP_1_ENABLED) == 0) {
							//TODO update ph pump 1
							ESP_LOGI("Updated ph pumps 1 to: ", "%s", pumps_element->valueint == 0 ? "false" : "true");
						} else if(strcmp(pumps_key, PUMP_2_ENABLED) == 0) {
							//TODO update ph pump 2
							ESP_LOGI("Updated ph pumps 2 to: ", "%s", pumps_element->valueint == 0 ? "false" : "true");
						}
						pumps_element = pumps_element->next;
					}
				}
				control_element = control_element->next;
			}
		}
		ESP_LOGI("", "next element");
		element = element->next;
	}
	ESP_LOGI("...", "about to commit");
	nvs_commit_data(handle);
	ESP_LOGI("", "Committed pH data to NVS");
	ESP_LOGI("", "Finished updating pH settings");
}
