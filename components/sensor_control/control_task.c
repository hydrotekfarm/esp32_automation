#include "control_task.h"

#include "task_manager.h"
#include "ph_control.h"
#include "ec_control.h"

void sensor_control (void *parameter) {
	reset_sensor_checks(&ph_checks);
	reset_sensor_checks(&ec_checks);

	target_ph = 5;
	target_ec = 4;

	ph_dose_time = 5;
	ph_wait_time = 10 * 60;
	ec_dose_time = 10;
	ec_wait_time = 10 * 60;
	ec_nutrient_proportions[0] = 0.10;
	ec_nutrient_proportions[1] = 0.20;
	ec_nutrient_proportions[2] = 0.30;
	ec_nutrient_proportions[3] = 0.10;
	ec_nutrient_proportions[4] = 0.00;
	ec_nutrient_proportions[5] = 0.30;

	for(;;)  {
		// Check sensors
		check_ph();
		check_ec();

		// Wait till next sensor readings
		vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}

void reset_sensor_checks(bool *sensor_checks) {
	for(int i = 0; i < sizeof(sensor_checks); i++) {
		sensor_checks[i] = false;
	}
}
