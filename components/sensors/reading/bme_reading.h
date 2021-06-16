#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include "sensor.h"

TaskHandle_t bme_task_handle;

struct sensor temperature_sensor;

struct sensor humidity_sensor; 

// Get temperature sensor
struct sensor* get_temperature_sensor();

//Get humidity sensor
struct sensor * get_humidity_sensor();

// Measures bme sensor (temperature and humidity)
void measure_bme();