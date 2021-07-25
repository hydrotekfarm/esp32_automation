#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "sensor.h"
#include "ph_sensor.h"

struct sensor ph_sensor;

ph_sensor_t dev;

// Get ph sensor
struct sensor* get_ph_sensor();

//Get ph dev 
ph_sensor_t* get_ph_dev();

// Measures water ph
void measure_ph();


