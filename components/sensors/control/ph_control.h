#include <stdbool.h>
#include <cjson.h>

// Margin of error
static const float PH_MARGIN_ERROR = 0.3;

// Active control status
bool ph_control_active;

// Day night custom control
bool ph_day_night_control;

// Target value
float target_ph;

// Night target values
float night_target_ph;

// Checks needed until dose
bool ph_checks[6];

// Timings
float ph_dose_time;
float ph_wait_time;

// Checks and adjust ph
void check_ph();

// Turn ph up pump on
void ph_up_pump();

// Turn ph down pump on
void ph_down_pump();

// Turn ph pumps off
void ph_pump_off();

// Update settings
void ph_update_settings(cJSON *item);
