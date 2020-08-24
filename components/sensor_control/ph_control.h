#include <stdbool.h>

// Target value
float target_ph = 5;

// Margin of error
float ph_margin_error = 0.3;

// Checks needed until dose
bool ph_checks[6] = {false, false, false, false, false, false};

// Timings
float ph_dose_time = 5;
float ph_wait_time = 10 * 60;

// Checks and adjust ph
void check_ph();

// Turn ph up pump on
void ph_up_pump();

// Turn ph down pump on
void ph_down_pump();

// Turn ph pumps off
void ph_pump_off();
