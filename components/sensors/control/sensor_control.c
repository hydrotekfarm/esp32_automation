#include "sensor_control.h"

void init_sensor_control(struct sensor_control *control_in, bool is_active_in, bool target_value_in, float margin_error_in,
						float night_target_value_in, bool is_day_night_in, float dose_time_in, float wait_time_in) {

	control_in->is_control_active = is_active_in;
	control_in->target_value = target_value_in;
	control_in->margin_error = margin_error_in;
	control_in->night_target_value = night_target_value_in;
	control_in->is_day_night_active = is_day_night_in;
	control_in->dose_time = dose_time_in;
	control_in->wait_time = wait_time_in;

	control_reset_checks(control_in);
}

bool control_get_active(struct sensor_control *control_in) { return control_in->is_control_active; }
void control_set_active(struct sensor_control *control_in, bool status) { control_in->is_control_active = status; }

float control_get_target(struct sensor_control *control_in) { return control_in->target_value; }
void control_set_target(struct sensor_control *control_in, float target) { control_in->target_value = target; }

bool control_get_day_night_active(struct sensor_control *control_in) { return control_in->is_day_night_active; }
void control_set_day_night_active(struct sensor_control *control_in, bool status) { control_in->is_day_night_active = status; }

float control_get_night_target(struct sensor_control *control_in) { return control_in->night_target_value; }
void control_set_night_target(struct sensor_control *control_in, float target) { control_in->night_target_value = target; }

float control_get_dose_time(struct sensor_control *control_in) { return control_in->dose_time; }
void control_set_dose_time(struct sensor_control *control_in, float time) { control_in->dose_time = time; }

float control_get_wait_time(struct sensor_control *control_in) { return control_in->wait_time; }
void control_set_wait_time(struct sensor_control *control_in, float time) { control_in->wait_time = time; }

bool control_add_check(struct sensor_control *control_in) {
	if(control_in->check_index == (NUM_CHECKS - 2)) {
		control_reset_checks(control_in);
		return true;
	}
	control_in->sensor_checks[control_in->check_index++] = true;
	return false;
}

void control_reset_checks(struct sensor_control *control_in) {
	for(int i = 0; i < NUM_CHECKS; i++) control_in->sensor_checks[i] = false;
	control_in->check_index = 0;
}
