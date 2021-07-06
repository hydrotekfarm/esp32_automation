#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "sensor.h"
#include "grow_manager.h"
#include "mqtt_manager.h"

struct sensor ec_sensor;

bool dry_calib;

// Get sensor object
struct sensor* get_ec_sensor();

// Measures water ph
void measure_ec();
