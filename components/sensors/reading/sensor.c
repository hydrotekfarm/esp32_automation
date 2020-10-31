#include <stdbool.h>
#include "sensor.h"

void init_sensor(struct sensor *sensor_in, bool active_in, bool calib_in) {
	sensor_in->current_value = 0;
	sensor_in->is_active = active_in;
	sensor_in->is_calib = calib_in;
}

TaskHandle_t sensor_get_task_handle(struct sensor *sensor_in) { return sensor_in->task_handle; }

double sensor_get_value(struct sensor *sensor_in) { return sensor_in->current_value; }
void sensor_set_value(struct sensor *sensor_in, double value) { sensor_in->current_value = value; }

bool sensor_active_status(struct sensor *sensor_in) { return sensor_in->is_active; }
void sensor_set_active_status(struct sensor *sensor_in, bool status) { sensor_in->is_active = status; }

bool sensor_calib_status(struct sensor *sensor_in) { return sensor_in->is_calib; }
void sensor_set_calib_status(struct sensor *sensor_in, bool status) { sensor_in->is_calib = status; }
