#include "ec_control.h"

#include <stdbool.h>
#include <esp_log.h>

#include "ph_control.h"
#include "rtc.h"
#include "ec_reading.h"
#include "control_task.h"
#include "sync_sensors.h"

void check_ec() {
	char *TAG = "EC_CONTROL";

	// Check if ph and ec is currently being altered
	bool ec_control = ec_dose_timer.active || ec_wait_timer.active;
	bool ph_control = ph_dose_timer.active || ph_wait_timer.active;

	if(!ec_control && !ph_control) {
		if(_ec < target_ec - EC_MARGIN_ERROR) {
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
		} else if(_ec > target_ec + EC_MARGIN_ERROR) {
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
