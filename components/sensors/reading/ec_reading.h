#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "sensor.h"
#include "ec_sensor.h"

struct sensor ec_sensor;

ec_sensor_t dev;

bool dry_calib;

// Get sensor object
struct sensor* get_ec_sensor();

//Get ec dev 
ec_sensor_t* get_ec_dev();

// Measures water ph
void measure_ec();
