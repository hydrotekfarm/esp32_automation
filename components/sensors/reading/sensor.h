#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "i2cdev.h"

#ifndef COMPONENTS_SENSORS_READING_SENSOR_H_
#define COMPONENTS_SENSORS_READING_SENSOR_H_

struct sensor {
	char name[25];
	TaskHandle_t task_handle;
	double current_value;
	bool is_active;
	bool is_calib;
};

#endif

// REQUIES: length of name_in must be <= 25 characters
// Initialize sensor
void init_sensor(struct sensor *sensor_in, char *name_in, bool active_in, bool calib_in);

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

// Calibrate sensor
void calibrate_sensor(struct sensor *sensor_in, esp_err_t (*calib_func)(i2c_dev_t*), i2c_dev_t *dev);
