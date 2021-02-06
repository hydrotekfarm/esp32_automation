#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sensor.h"

struct sensor ph_sensor;

// Get ph sensor
struct sensor* get_ph_sensor();

// Measures water ph
void measure_ph();
