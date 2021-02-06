#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sensor.h"

struct sensor ec_sensor;

bool dry_calib;

// Get sensor object
struct sensor* get_ec_sensor();

// Measures water ph
void measure_ec();
