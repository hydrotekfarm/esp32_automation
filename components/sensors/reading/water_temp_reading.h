#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "sensor.h"

// Water temperature sensor
struct sensor water_temp_sensor;

// Get sensor
struct sensor *get_water_temp_sensor();

// Measures water temperature
void measure_water_temperature();
