#include "ec_control.h"

#include <stdbool.h>
#include <esp_log.h>
#include <esp_err.h>
#include <string.h>

#include "control_settings_keys.h"
#include "sensor_control.h"
#include "ph_control.h"
#include "rtc.h"
#include "ec_reading.h"
#include "control_task.h"
#include "sync_sensors.h"
#include "ports.h"

struct sensor_control* get_ec_control() { return &ec_control; }

void check_ec() {
	if(!control_get_active(get_ph_control())) {
		int result = control_check_sensor(&ec_control, sensor_get_value(get_ec_sensor()));
		if(result == -1) {
			ec_nutrient_index = 0;
			ec_dose();
		} else if(result == 1) {
			// TODO dilute ec with water
		}
	}
}

void ec_dose() {
	// Check if last nutrient was pumped
	if(ec_nutrient_index == EC_NUM_PUMPS) {
		// Turn off last pump
		set_gpio_off(ec_pump_gpios[ec_nutrient_index - 1]);

		// Enable wait timer and reset nutrient index
		control_start_wait_timer(&ec_control);
		ec_nutrient_index = 0;
		ESP_LOGI(EC_TAG, "EC dosing done");

		// Still nutrients left
	} else {
		// Turn off last pump as long as this isn't first pump and turn on next pump
		if(ec_nutrient_index != 0) set_gpio_off(ec_pump_gpios[ec_nutrient_index - 1]);

		// Only dose if dosing proportions > 0
		if(ec_nutrient_proportions[ec_nutrient_index] > 1e-4) {
			set_gpio_on(ec_pump_gpios[ec_nutrient_index]);

			// Enable dose timer based on nutrient proportion
			control_set_dose_percentage(&ec_control, ec_nutrient_proportions[ec_nutrient_index]);
			control_start_dose_timer(&ec_control);
			ESP_LOGI(EC_TAG, "Dosing nutrient %d for %.2f seconds", ec_nutrient_index  + 1, control_get_dose_time(&ec_control));
			ec_nutrient_index++;
		} else {
			ec_nutrient_index++;
			ec_dose();
		}
	}
}

void ec_update_settings(cJSON *item) {
	nvs_handle_t *handle = nvs_get_handle(EC_NAMESPACE);
	control_update_settings(&ec_control, item, handle);

	cJSON *element = item->child;
	while(element != NULL) {
		char *key = element->string;
		if(strcmp(key, CONTROL) == 0) {
			cJSON *control_element = element->child;
			while(control_element != NULL) {
				char *control_key = control_element->string;
				if(strcmp(control_key, PUMPS) == 0) {
					cJSON *pumps_element = control_element->child;
					while(pumps_element != NULL) {
						char *pumps_key = pumps_element->string;
						uint8_t pump_num = pumps_key[PUMP_NUM_INDEX] - '1';
						float pump_val = pumps_element->valuedouble;

						ec_nutrient_proportions[pump_num] = pump_val;
						ESP_LOGI("Updated ec pump", "%d to: %f", pump_num+1, pump_val);
						nvs_add_float(handle, pumps_key, pump_val);

						pumps_element = pumps_element->next;
					}
				}
				control_element = control_element->next;
			}
		}
		element = element->next;
	}

	nvs_commit_data(handle);
	ESP_LOGI(EC_TAG, "Updating settings and committed data to NVS");
}

void ec_get_nvs_settings() {
	control_get_nvs_settings(&ec_control, EC_NAMESPACE);

	// Get ec proportions from NVS
	size_t num_index = strlen(PUMP_NUM);
	char *key = malloc((num_index + 2) * sizeof(char));
	key[strlen(key)] = '\0';

	for(int i = 0; i < EC_NUM_PUMPS; ++i) {
		key[num_index] = i + '1';
		//nvs_get_float(EC_NAMESPACE, key, &ec_nutrient_proportions[i]);
	}

	free(key);

	ESP_LOGI(EC_TAG, "Updated settings from NVS");
}
