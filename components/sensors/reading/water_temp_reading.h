#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sensor.h"

// Water temperature sensor
struct sensor water_temp_sensor;

// Get sensor
struct sensor *get_water_temp_sensor();

// Measures water temperature
void measure_water_temperature();
