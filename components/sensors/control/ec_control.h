#include <stdbool.h>
#include <esp_system.h>

// Margin of error
#define EC_MARGIN_ERROR 0.5

// Number of pumps
#define EC_NUM_PUMPS 6

// Target ec
float target_ec;

// Checks needed until dose
bool ec_checks[6];

// Timings
float ec_dose_time;
float ec_wait_time;

// Index of current pump
uint32_t ec_nutrient_index;

// Percent split of pumps
float ec_nutrient_proportions[6];

// GPIOS of pumps
uint32_t ec_pump_gpios[6];

// Check ec and adjust accordingly
void check_ec();

// Dose ec nutrients based on proportions
void ec_dose();
