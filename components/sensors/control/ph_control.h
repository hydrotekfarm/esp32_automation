#include <stdbool.h>
#include <cjson.h>
#include "sensor_control.h"

#define PH_TAG "PH_CONTROL"

// Margin of error
static const float PH_MARGIN_ERROR = 0.3;

// Control struct
struct sensor_control ph_control;

// Get control
struct sensor_control* get_ph_control();

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

// Get and store settings from NVS
void ph_get_nvs_settings();
