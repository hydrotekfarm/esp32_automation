#include <stdbool.h>
#include <esp_system.h>
#include <cjson.h>
#include "sensor_control.h"

// Margin of error
static const float EC_MARGIN_ERROR = 0.5;

// Number of pumps
#define EC_NUM_PUMPS 6

// Control struct
struct sensor_control ec_control;

// Get control struct
struct sensor_control* get_ec_control();

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

// Update settings
void ec_update_settings(cJSON *item);
