#include "ph_control.h"

#include <stdbool.h>
#include <esp_log.h>

#include "rtc.h"
#include "ph_reading.h"
#include "control_task.h"
#include "sync_sensors.h"
#include "ports.h"

void check_ph() { // Check ph
	char *TAG = "PH_CONTROL";

	// Check if ph and ec is currently being altered
	bool ph_control = ph_dose_timer.active || ph_wait_timer.active;
	bool ec_control = ec_dose_timer.active || ec_wait_timer.active;

	// Only proceed if ph and ec are not being altered
	if(!ph_control && !ec_control) {
		// Check if ph is too low
		if(_ph < target_ph - PH_MARGIN_ERROR) {
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
		} else if(_ph > target_ph + PH_MARGIN_ERROR) {
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
	gpio_set_level(PH_UP_PUMP_GPIO, 1);
	ESP_LOGI("", "pH up pump on");

	// Enable dose timer
	enable_timer(&dev, &ph_dose_timer, ph_dose_time);
}

void ph_down_pump() {
	gpio_set_level(PH_DOWN_PUMP_GPIO, 1);
	ESP_LOGI("", "pH down pump on");

	// Enable dose timer
	enable_timer(&dev, &ph_dose_timer, ph_dose_time);
}

void ph_pump_off() {
	gpio_set_level(PH_UP_PUMP_GPIO, 0);
	gpio_set_level(PH_DOWN_PUMP_GPIO, 0);
	ESP_LOGI("", "pH pumps off");

	// Enable wait timer
	enable_timer(&dev, &ph_wait_timer, ph_wait_time - (sizeof(ph_checks) * (SENSOR_MEASUREMENT_PERIOD  / 1000))); // Offset wait time based on amount of checks and check duration
}
