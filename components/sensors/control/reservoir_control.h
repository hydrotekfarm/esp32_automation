#include <stdbool.h>

bool reservoir_control_active;
bool reservoir_change_flag;
bool top_float_switch_trigger = false;
bool bottom_float_switch_trigger = false;

void set_reservoir_change_flag();
