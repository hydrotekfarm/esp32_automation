#include "reservoir_control.h"

#include <esp_log.h>

#include "ph_control.h"
#include "rtc.h"
#include "ec_reading.h"
#include "control_task.h"
#include "sync_sensors.h"
#include "JSON_keys.h"

void set_reservoir_change_flag() {
	reservoir_change_flag = true;
}

void reset_reservoir_change_flag() {
	reservoir_change_flag = false;
}

void check_water_level() {
	char *TAG = "RESERVOIR_CONTROL";

	// Check if ph and ec is currently being altered
	bool ec_control = ec_dose_timer.active || ec_wait_timer.active;
	bool ph_control = ph_dose_timer.active || ph_wait_timer.active;

	if(!ec_control || !ph_control) {
		if(reservoir_change_flag) {
			// reservoir change code
			reset_reservoir_change_flag();
			return;
		}
		if(top_float_switch_trigger) {

			top_float_switch_trigger = false;
		}
	}
}
