#include <stdbool.h>

#ifndef COMPONENTS_SENSORS_CONTROL_SENSOR_CONTROL_H_
#define COMPONENTS_SENSORS_CONTROL_SENSOR_CONTROL_H_

#define NUM_CHECKS 6

struct sensor_control {
	char name[25];
	bool is_control_active;
	float target_value;
	float margin_error;
	bool is_day_night_active;
	float night_target_value;
	bool sensor_checks[NUM_CHECKS];
	int check_index;
	float dose_time;
	float wait_time;
};

#endif /* COMPONENTS_SENSORS_CONTROL_SENSOR_CONTROL_H_ */

// Initialize control structure
void init_sensor_control(struct sensor_control *control_in, char *name_in, bool is_active_in, bool target_value_in,
						float margin_error_in, float night_target_value_in, bool is_day_night_in);
void init_doser_control(struct sensor_control *control_in, float dose_time_in, float wait_time_in);

// Getters and setters
bool control_get_active(struct sensor_control *control_in);
void control_set_active(struct sensor_control *control_in, bool status);

void control_set_target(struct sensor_control *control_in, float target);

bool control_get_day_night_active(struct sensor_control *control_in);
void control_set_day_night_active(struct sensor_control *control_in, bool status);

void control_set_night_target(struct sensor_control *control_in, float target);

float control_get_dose_time(struct sensor_control *control_in);
void control_set_dose_time(struct sensor_control *control_in, float time);

float control_get_wait_time(struct sensor_control *control_in);
void control_set_wait_time(struct sensor_control *control_in, float time);

// Returns target value based on time of day
float control_get_target_value(struct sensor_control *control_in);

// Checks if sensor is out of range
bool control_is_under_target(struct sensor_control *control_in, float current_value);
bool control_is_over_target(struct sensor_control *control_in, float current_value);

// Checks sensor value and updates checks accordingly
// Returns 0 if sensor is fine, -1 if confirmed too low, and 1 if confirmed too high
int control_check_sensor(struct sensor_control *control_in, float current_value);

// Adds true to next sensor check
// Returns true if all checks are done, false otherwise
bool control_add_check(struct sensor_control *control_in);

void control_reset_checks(struct sensor_control *control_in);
