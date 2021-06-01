#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "co2_sensor_struct.h"

struct co2_sensor co2_sensor;

// Get co2 co2_sensor
struct co2_sensor* get_co2_sensor();

// Measures water ph
void measure_co2();
