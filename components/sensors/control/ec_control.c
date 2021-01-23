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
		ESP_LOGI("", "EC dosing done");

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
			ESP_LOGI("", "Dosing nutrient %d for %.2f seconds", ec_nutrient_index  + 1, control_get_dose_time(&ec_control));
			ec_nutrient_index++;
		} else {
			ec_nutrient_index++;
			ec_dose();
		}
	}
}

void ec_update_settings(cJSON *item) {
	struct NVS_Data *data = nvs_init_data();
	control_update_settings(&ec_control, item, data);

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
						if(strcmp(pumps_key, PUMP_1) == 0) {
							cJSON *pump1_element = pumps_element->child;
							while(pump1_element != NULL) {
								char *pump1_key = pump1_element->string;
								if(strcmp(pump1_key, ENABLED)) {
									//TODO update ec pump 1 status
									ESP_LOGI("Updated ec pump 1 to: ", "%s", pump1_element->valueint == 0 ? "false" : "true");
								} else if(strcmp(pump1_key, VALUE)) {
									//TODO update ec pump 1 value
									ESP_LOGI("Updated ec pump 1 value to: ", "%d", pump1_element->valueint);
								}
								pump1_element = pump1_element->next;
							}
						} else if(strcmp(pumps_key, PUMP_2) == 0) {
							cJSON *pump2_element = pumps_element->child;
							while(pump2_element != NULL) {
								char *pump2_key = pump2_element->string;
								if(strcmp(pump2_key, ENABLED)) {
									//TODO update ec pump 2 status
									ESP_LOGI("Updated ec pump 2 to: ", "%s", pump2_element->valueint == 0 ? "false" : "true");
								} else if(strcmp(pump2_key, VALUE)) {
									//TODO update ec pump 2 value
									ESP_LOGI("Updated ec pump 2 value to: ", "%d", pump2_element->valueint);
								}
								pump2_element = pump2_element->next;
							}
						} else if(strcmp(pumps_key, PUMP_3) == 0) {
							cJSON *pump3_element = pumps_element->child;
							char *pump3_key = pump3_element->string;
							while(pump3_element != NULL) {
								if(strcmp(pump3_key, ENABLED)) {
									//TODO update ec pump 3 status
									ESP_LOGI("Updated ec pump 3 to: ", "%s", pump3_element->valueint == 0 ? "false" : "true");
								} else if(strcmp(pump3_key, VALUE)) {
									//TODO update ec pump 3 value
									ESP_LOGI("Updated ec pump 3 value to: ", "%d", pump3_element->valueint);
								}
								pump3_element = pump3_element->next;
							}
						} else if(strcmp(pumps_key, PUMP_4) == 0) {
							cJSON *pump4_element = pumps_element->child;
							char *pump4_key = pump4_element->string;
							while(pump4_element != NULL) {
								if(strcmp(pump4_key, ENABLED)) {
									//TODO update ec pump 4 status
									ESP_LOGI("Updated ec pump 4 to: ", "%s", pump4_element->valueint == 0 ? "false" : "true");
								} else if(strcmp(pump4_key, VALUE)) {
									//TODO update ec pump 4 value
									ESP_LOGI("Updated ec pump 4 value to: ", "%d", pump4_element->valueint);
								}
								pump4_element = pump4_element->next;
							}
						} else if(strcmp(pumps_key, PUMP_5) == 0) {
							cJSON *pump5_element = pumps_element->child;
							char *pump5_key = pump5_element->string;
							while(pump5_element != NULL) {
								if(strcmp(pump5_key, ENABLED)) {
									//TODO update ec pump 5 status
									ESP_LOGI("Updated ec pump 5 to: ", "%s", pump5_element->valueint == 0 ? "false" : "true");
								} else if(strcmp(pump5_key, VALUE)) {
									//TODO update ec pump 5 value
									ESP_LOGI("Updated ec pump 5 value to: ", "%d", pump5_element->valueint);
								}
								pump5_element = pump5_element->next;
							}
						}
						pumps_element = pumps_element->next;
					}
				}

				control_element = control_element->next;
			}
		}
		element = element->next;
	}

	nvs_commit_data(data, EC_NAMESPACE);
	ESP_LOGI("", "Committed ec data to NVS");
	ESP_LOGI("", "Finished updating ec settings");
}
