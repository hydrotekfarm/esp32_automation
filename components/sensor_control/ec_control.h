#include "port_manager.h"

// Target ec
float target_ec = 4;

// Margin of error
float ec_margin_error = 0.5;

// Checks needed until dose
bool ec_checks[6] = {false, false, false, false, false, false};

// Timings
float ec_dose_time = 10;
float ec_wait_time = 10 * 60;

// Number of pumps
uint32_t ec_num_pumps = 6;

// Index of current pump
uint32_t ec_nutrient_index = 0;

// Percent split of pumps
float ec_nutrient_proportions[6] = {0.10, 0.20, 0.30, 0.10, 0, 0.30};

// GPIOS of pumps
uint32_t ec_pump_gpios[6] = {EC_NUTRIENT_1_PUMP_GPIO, EC_NUTRIENT_2_PUMP_GPIO, EC_NUTRIENT_3_PUMP_GPIO, EC_NUTRIENT_4_PUMP_GPIO, EC_NUTRIENT_5_PUMP_GPIO, EC_NUTRIENT_6_PUMP_GPIO};

// Check ec and adjust accordingly
void check_ec();

// Dose ec nutrients based on proportions
void ec_dose();
