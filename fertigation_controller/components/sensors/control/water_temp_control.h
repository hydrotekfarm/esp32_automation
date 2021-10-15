#include <stdbool.h>
#include <cjson.h>

#include "sensor_control.h"

#define WATER_TEMP_TAG "WATER_TEMP_CONTROL"

// Margin of error
static const float WATER_TEMP_MARGIN_ERROR = 5;

// Control struct
struct sensor_control water_temp_control;

// Track when any water temperature equipment is on
bool is_water_cooler_on;

// Get control
struct sensor_control* get_water_temp_control();

// Checks and adjust water temperature
void check_water_temp();

// Turn water heater on
void heat_water();

// Turn water cooler on
void cool_water();

// Turn ph pumps off
void stop_water_adjustment();

// Update settings
void water_temp_update_settings(cJSON *item);

// Get and store settings from NVS
void water_temp_get_nvs_settings();