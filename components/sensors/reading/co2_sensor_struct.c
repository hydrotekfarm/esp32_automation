#include <stdbool.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include "co2_sensor_struct.h"


void init_sensor(struct co2_sensor *sensor_in, char *name_in, bool active_in, bool calib_in) {
	strcpy(sensor_in->name, name_in);
	sensor_in->current_value = 0;
	sensor_in->is_active = active_in;
	sensor_in->is_calib = calib_in;
}

TaskHandle_t* sensor_get_task_handle(struct co2_sensor *sensor_in) { return &sensor_in->task_handle; }

unsigned int sensor_get_value(const struct co2_sensor *sensor_in) { return sensor_in->current_value; }
unsigned int* sensor_get_address_value(struct co2_sensor *sensor_in) {	return &sensor_in->current_value; }
void sensor_set_value(struct co2_sensor *sensor_in, unsigned value) { sensor_in->current_value = value; }

bool sensor_get_active_status(struct co2_sensor *sensor_in) { return sensor_in->is_active; }
void sensor_set_active_status(struct co2_sensor *sensor_in, bool status) { sensor_in->is_active = status; }

bool sensor_calib_status(struct co2_sensor *sensor_in) { return sensor_in->is_calib; }
void sensor_set_calib_status(struct co2_sensor *sensor_in, bool status) { sensor_in->is_calib = status; }


void sensor_get_json(struct co2_sensor *sensor_in, cJSON **obj) {
	*obj = cJSON_CreateObject();
	cJSON *name, *value;

	name = cJSON_CreateString(sensor_in->name);

	char value_str[8];
	snprintf(value_str, sizeof(value_str), "%d", sensor_in->current_value);
	value = cJSON_CreateString(value_str);

	cJSON_AddItemToObject(*obj, "name", name);
	cJSON_AddItemToObject(*obj, "value", value);
}
