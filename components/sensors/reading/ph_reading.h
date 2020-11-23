#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "sensor.h"

struct sensor ph_sensor;

// Get ph sensor
const struct sensor* get_ph_sensor();

// Measures water ph
void measure_ph();
