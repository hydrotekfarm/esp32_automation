#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sensor.h"
#include "ec_sensor.h"
#include "time.h"

struct sensor ec_sensor;

ec_sensor_t ec_dev;

bool dry_calib;

//variable to check if ec sensor is activated
bool is_ec_activated;

//get status of is_activated
bool get_is_ec_activated();

//set is_activated variable
void set_is_ec_activated(bool is_active);

// Get sensor object
struct sensor* get_ec_sensor();

//Get ec dev 
ec_sensor_t* get_ec_dev();

// Measures water ph
void measure_ec();

// to calculate time delay 
double start, end, exec_time;