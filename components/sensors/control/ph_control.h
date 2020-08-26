#include <stdbool.h>

// Margin of error
#define PH_MARGIN_ERROR 0.3

// Active control status
bool ph_control_active;

// Target value
float target_ph;

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
