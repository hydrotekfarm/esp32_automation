#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "sensor.h"

struct sensor ec_sensor;

// Get sensor object
const struct sensor* get_ec_sensor();

// Measures water ph
void measure_ec();
