#include "control_task.h"

#include "ph_control.h"
#include "ec_control.h"
#include "sync_sensors.h"
#include "ports.h"

void sensor_control (void *parameter) {
	reset_sensor_checks(&ph_checks);
	reset_sensor_checks(&ec_checks);

	ph_control_active = true;
	ec_control_active = true;

	target_ph = 6;
	target_ec = 4;

	ph_dose_time = 5;
	ph_wait_time = 5 * 60;
	ec_dose_time = 10;
	ec_wait_time = 5 * 60;

	ec_nutrient_proportions[0] = 0.10;
	ec_nutrient_proportions[1] = 0.20;
	ec_nutrient_proportions[2] = 0.30;
	ec_nutrient_proportions[3] = 0.10;
	ec_nutrient_proportions[4] = 0.00;
	ec_nutrient_proportions[5] = 0.30;

	ec_pump_gpios[0] = EC_NUTRIENT_1_PUMP_GPIO;
	ec_pump_gpios[1] = EC_NUTRIENT_2_PUMP_GPIO;
	ec_pump_gpios[2] = EC_NUTRIENT_3_PUMP_GPIO;
	ec_pump_gpios[3] = EC_NUTRIENT_4_PUMP_GPIO;
	ec_pump_gpios[4] = EC_NUTRIENT_5_PUMP_GPIO;
	ec_pump_gpios[5] = EC_NUTRIENT_6_PUMP_GPIO;

	for(;;)  {
		// Check sensors
		if(ph_control_active) check_ph();
		if(ec_control_active) check_ec();

		// Wait till next sensor readings
		vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}

void reset_sensor_checks(bool *sensor_checks) {
	for(int i = 0; i < sizeof(sensor_checks); i++) {
		sensor_checks[i] = false;
	}
}
