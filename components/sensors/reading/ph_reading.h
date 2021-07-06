#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "sensor.h"
#include "grow_manager.h"
#include "mqtt_manager.h"

struct sensor ph_sensor;

// Get ph sensor
struct sensor* get_ph_sensor();

// Measures water ph
void measure_ph();


