#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "sensor.h"
#include "ph_sensor.h"

struct sensor ph_sensor;

ph_sensor_t dev;

//variable to check if ph sensor is activated
bool is_ph_activated;

//get status of is_activated
bool get_is_ph_activated();

//set is_activated variable
void set_is_ph_activated(bool is_active);

// Get ph sensor
struct sensor* get_ph_sensor();

//Get ph dev 
ph_sensor_t* get_ph_dev();

// Measures water ph
void measure_ph();


