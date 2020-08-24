#include "control_task.h"

#include "task_manager.h"
#include "ph_control.h"
#include "ec_control.h"

void sensor_control (void *parameter) {
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
