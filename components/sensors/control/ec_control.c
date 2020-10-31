#include "ec_control.h"

#include <stdbool.h>
#include <esp_log.h>
#include <string.h>

#include "ph_control.h"
#include "rtc.h"
#include "ec_reading.h"
#include "control_task.h"
#include "sync_sensors.h"
#include "JSON_keys.h"

void check_ec() {
	char *TAG = "EC_CONTROL";

	// Set current target based on time of day
	float current_target = !is_day && ec_day_night_control ? night_target_ec : target_ec;

	// Check if ph and ec is currently being altered
	bool ec_control = ec_dose_timer.active || ec_wait_timer.active;
	bool ph_control = ph_dose_timer.active || ph_wait_timer.active;

	if(!ec_control && !ph_control) {
		if(sensor_get_value(get_ec_sensor()) < current_target - EC_MARGIN_ERROR) {
			// Check if all checks are complete
			if(ec_checks[sizeof(ec_checks) - 1]) {
				ec_nutrient_index = 0;
				ec_dose();
				reset_sensor_checks(ec_checks);
			} else {
				// Iterate through checks
				for(int i = 0; i < sizeof(ec_checks); i++) {
					// Once false check is reached, set it to true and leave loop
					if(!ec_checks[i]) {
						ec_checks[i] = true;
						ESP_LOGI(TAG, "EC check %d done", i+1);
						break;
					}
				}
			}
		} else if(sensor_get_value(get_ec_sensor()) > current_target + EC_MARGIN_ERROR) {
			// Check if all checks are complete
			if(ec_checks[sizeof(ec_checks) - 1]) {
				// TODO dilute ec with  water
				reset_sensor_checks(ec_checks);
			} else {
				// Iterate through checks
				for(int i = 0; i < sizeof(ec_checks); i++) {
					// Once false check is reached, set it to true and leave loop
					if(!ec_checks[i]) {
						ec_checks[i] = true;
						ESP_LOGI(TAG, "EC check %d done", i+1);
						break;
					}
				}
			}
		}
		// Reset sensor checks if ph is active
	} else if(ph_control) {
		reset_sensor_checks(ec_checks);
	}
}

void ec_dose() {
	// Check if last nutrient was pumped
	if(ec_nutrient_index == EC_NUM_PUMPS) {
		// Turn off last pump
		gpio_set_level(ec_pump_gpios[ec_nutrient_index - 1], 0);

		// Enable wait timer and reset nutrient index
		enable_timer(&dev, &ec_wait_timer, ec_wait_time - (sizeof(ec_checks) * (SENSOR_MEASUREMENT_PERIOD  / 1000))); // Offset timer based on check durations
		ec_nutrient_index = 0;
		ESP_LOGI("", "EC dosing done");

		// Still nutrients left
	} else {
		// Turn off last pump as long as this isn't first pump and turn on next pump
		if(ec_nutrient_index != 0) gpio_set_level(ec_pump_gpios[ec_nutrient_index - 1], 0);
		gpio_set_level(ec_pump_gpios[ec_nutrient_index], 1);

		// Enable dose timer based on nutrient proportion
		enable_timer(&dev, &ec_dose_timer, ec_dose_time * ec_nutrient_proportions[ec_nutrient_index]);
		ESP_LOGI("", "Dosing nutrient %d for %.2f seconds", ec_nutrient_index  + 1, ec_dose_time * ec_nutrient_proportions[ec_nutrient_index]);
		ec_nutrient_index++;
	}
}

void ec_update_settings(cJSON *item) {
	cJSON *element = item->child;
	while(element != NULL) {
		char *key = element->string;
		if(strcmp(key, MONITORING_ONLY) == 0) {
			//TODO update monitoring only variable
			ESP_LOGI("Updated monitoring only to: ", "%s", element->valueint == 0 ? "false" : "true");
		} else if(strcmp(key, CONTROL) == 0) {
			cJSON *control_element = element->child;
			while(control_element != NULL) {
				char *control_key = control_element->string;
				if(strcmp(control_key, DOSING_TIME) == 0) {
					//TODO update dosing time
					ESP_LOGI("Updated ec dosing time to: ", "%d", control_element->valueint);
				} else if(strcmp(control_key, DOSING_INTERVAL) == 0) {
					//TODO update dosing interval
					ESP_LOGI("Updated ec dosing interval to: ", "%d", control_element->valueint);
				} else if(strcmp(control_key, DAY_AND_NIGHT) == 0) {
					//TODO update day and night
					ESP_LOGI("Updated ec day and night to: ", "%s", control_element->valueint == 0 ? "false" : "true");
				} else if(strcmp(control_key, DAY_TARGET_VALUE) == 0) {
					//TODO update day target value
					ESP_LOGI("Updated ec day target value to: ", "%d", control_element->valueint);
				} else if(strcmp(control_key, NIGHT_TARGET_VALUE) == 0) {
					//TODO update night target value
					ESP_LOGI("Updated ec night target value: ", "%d", control_element->valueint);
				} else if(strcmp(control_key, TARGET_VALUE) == 0) {
					//TODO update target value
					ESP_LOGI("Updated ec target value to: ", "%d", control_element->valueint);
				} else if(strcmp(control_key, PUMPS) == 0) {
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
		} else if(strcmp(key, ALARM_MIN) == 0) {
			//TODO update alarm min
			ESP_LOGI("Updated ec alarm min to: ", "%d", element->valueint);
		} else if(strcmp(key, ALARM_MAX) == 0) {
			//TODO update alarm min
			ESP_LOGI("Updated ec alarm max to: ", "%d", element->valueint);
		}

		element = element->next;
	}
	ESP_LOGI("", "Finished updating ec settings");
}
