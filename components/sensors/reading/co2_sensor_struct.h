#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include <cjson.h>
#include "i2cdev.h"

#ifndef COMPONENTS_CO2_SENSORS_READING_SENSOR_H_
#define COMPONENTS_CO2_SENSORS_READING_SENSOR_H_

struct co2_sensor {
	char name[25];
	TaskHandle_t task_handle;
	unsigned int current_value;
	bool is_active;
	bool is_calib;
};

#endif

// REQUIES: length of name_in must be <= 25 characters
// Initialize co2_sensor
void init_sensor(struct co2_sensor *sensor_in, char *name_in, bool active_in, bool calib_in);

// Get co2_sensor task handle
TaskHandle_t* sensor_get_task_handle(struct co2_sensor *sensor_in);

// Get and set current value
unsigned int sensor_get_value(const struct co2_sensor *sensor_in);
unsigned int* sensor_get_address_value(struct co2_sensor *sensor_in);
void sensor_set_value(struct co2_sensor *sensor_in, unsigned int value);

// Get and set current active status
bool sensor_get_active_status(struct co2_sensor *sensor_in);
void sensor_set_active_status(struct co2_sensor *sensor_in, bool status);

// Get and set calibration status
bool sensor_calib_status(struct co2_sensor *sensor_in);
void sensor_set_calib_status(struct co2_sensor *sensor_in, bool status);


// Get JSON object of co2_sensor dat
void sensor_get_json(struct co2_sensor *sensor_in, cJSON **obj);
