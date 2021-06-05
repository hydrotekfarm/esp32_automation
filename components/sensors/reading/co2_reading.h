#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "sensor.h"

struct sensor co2_sensor;

// Get co2 co2_sensor
struct sensor* get_co2_sensor();

// Measures water ph
void measure_co2();
