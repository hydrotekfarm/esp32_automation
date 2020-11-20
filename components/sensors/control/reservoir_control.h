#include <stdbool.h>
#include "rf_transmitter.h"

bool reservoir_control_active;
bool reservoir_change_flag;
bool top_float_switch_trigger;
bool bottom_float_switch_trigger;

struct rf_message water_in_rf_message;
struct rf_message water_out_rf_message;

void set_reservoir_change_flag(bool active);

void check_water_level();
