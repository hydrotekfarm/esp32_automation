#include "ph_control.h"

#include <stdbool.h>
#include <esp_log.h>
#include <string.h>

#include "rtc.h"
#include "ph_reading.h"
#include "control_task.h"
#include "sync_sensors.h"
#include "ports.h"
#include "JSON_keys.h"

void check_ph() { // Check ph
	char *TAG = "PH_CONTROL";

	// Set current target based on time of day
	float current_target = !is_day && ph_day_night_control ? night_target_ph : target_ph;

	// Check if ph and ec is currently being altered
	bool ph_control = ph_dose_timer.active || ph_wait_timer.active;
	bool ec_control = ec_dose_timer.active || ec_wait_timer.active;

	// Only proceed if ph and ec are not being altered
	if(!ph_control && !ec_control) {
		// Check if ph is too low
		if(sensor_get_value(get_ph_sensor()) < current_target - PH_MARGIN_ERROR) {
			// Check if all checks are complete
			if(ph_checks[sizeof(ph_checks) - 1]) {
				// Turn pump on and reset checks
				ph_up_pump();
				reset_sensor_checks(ph_checks);
			} else {
				// Iterate through checks
				for(int i = 0; i < sizeof(ph_checks); i++) {
					// Once false check is reached, set it to true and leave loop
					if(!ph_checks[i]) {
						ph_checks[i] = true;
						ESP_LOGI(TAG, "PH check %d done", i+1);
						break;
					}
				}
			}
			// Check if ph is too high
		} else if(sensor_get_value(get_ph_sensor()) > current_target + PH_MARGIN_ERROR) {
			// Check if ph checks are complete
			if(ph_checks[sizeof(ph_checks) - 1]) {
				// Turn pump on and reset checks
				ph_down_pump();
				reset_sensor_checks(ph_checks);
			} else {
				// Iterate through checks
				for(int i = 0; i < sizeof(ph_checks); i++) {
					// Once false check is reached, set it to true and leave loop
					if(!ph_checks[i]) {
						ph_checks[i] = true;
						ESP_LOGI(TAG, "PH check %d done", i+1);
						break;
					}
				}
			}
		} else {
			// Reset checks if ph is good
			reset_sensor_checks(ph_checks);
		}
		// Reset sensor checks if ec is active
	} else if(ec_control) {
		reset_sensor_checks(ph_checks);
	}
}

void ph_up_pump() {
	set_gpio_on(PH_UP_PUMP_GPIO);
	ESP_LOGI("", "pH up pump on");

	// Enable dose timer
	ESP_LOGI("sdf", "%p", &dev);
	enable_timer(&dev, &ph_dose_timer, ph_dose_time);
}

void ph_down_pump() {
	set_gpio_on(PH_DOWN_PUMP_GPIO);
	ESP_LOGI("", "pH down pump on");

	// Enable dose timer
	enable_timer(&dev, &ph_dose_timer, ph_dose_time);
}

void ph_pump_off() {
	set_gpio_off(PH_UP_PUMP_GPIO);
	set_gpio_off(PH_DOWN_PUMP_GPIO);
	ESP_LOGI("", "pH pumps off");

	// Enable wait timer
	enable_timer(&dev, &ph_wait_timer, ph_wait_time - (sizeof(ph_checks) * (SENSOR_MEASUREMENT_PERIOD  / 1000))); // Offset wait time based on amount of checks and check duration
}

void ph_update_settings(cJSON *item) {
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
				if(strcmp(control_key, PH_UP_DOWN) == 0) {
					//TODO update ph up down
					ESP_LOGI("Updated ph up and down to: ", "%s", control_element->valueint == 0 ? "false" : "true");
				} else if(strcmp(control_key, DOSING_TIME) == 0) {
					//TODO update dosing time
					ESP_LOGI("Updated ph dosing time to: ", "%d", control_element->valueint);
				} else if(strcmp(control_key, DOSING_INTERVAL) == 0) {
					//TODO update dosing interval
					ESP_LOGI("Updated ph dosing interval to: ", "%d", control_element->valueint);
				} else if(strcmp(control_key, DAY_AND_NIGHT) == 0) {
					//TODO update day and night
					ESP_LOGI("Updated ph day and night to: ", "%s", control_element->valueint == 0 ? "false" : "true");
				} else if(strcmp(control_key, DAY_TARGET_VALUE) == 0) {
					//TODO update day target value
					ESP_LOGI("Updated ph day target value to: ", "%d", control_element->valueint);
				} else if(strcmp(control_key, NIGHT_TARGET_VALUE) == 0) {
					//TODO update night target value
					ESP_LOGI("Updated ph night target value: ", "%d", control_element->valueint);
				} else if(strcmp(control_key, TARGET_VALUE) == 0) {
					//TODO update target value
					ESP_LOGI("Updated ph target value to: ", "%d", control_element->valueint);
				} else if(strcmp(control_key, PUMPS) == 0) {
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
		} else if(strcmp(key, ALARM_MIN) == 0) {
			//TODO update alarm min
			ESP_LOGI("Updated ph alarm min to: ", "%d", element->valueint);
		} else if(strcmp(key, ALARM_MAX) == 0) {
			//TODO update alarm min
			ESP_LOGI("Updated ph alarm max to: ", "%d", element->valueint);
		}

		element = element->next;
	}
	ESP_LOGI("", "Finished updating pH settings");
}
