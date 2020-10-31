#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>

#ifndef COMPONENTS_SENSORS_READING_SENSOR_H_
#define COMPONENTS_SENSORS_READING_SENSOR_H_

struct sensor {
	TaskHandle_t task_handle;
	double current_value;
	bool is_active;
	bool is_calib;
};

#endif

// Initialize sensor
void init_sensor(struct sensor *sensor_in, bool active_in, bool calib_in);

// Get sensor task handle
TaskHandle_t* sensor_get_task_handle(struct sensor *sensor_in);

// Get and set current value
double sensor_get_value(const struct sensor *sensor_in);
double* sensor_get_address_value(struct sensor *sensor_in);
void sensor_set_value(struct sensor *sensor_in, double value);

// Get and set current active status
bool sensor_active_status(struct sensor *sensor_in);
void sensor_set_active_status(struct sensor *sensor_in, bool status);

// Get and set calibration status
bool sensor_calib_status(struct sensor *sensor_in);
void sensor_set_calib_status(struct sensor *sensor_in, bool status);
