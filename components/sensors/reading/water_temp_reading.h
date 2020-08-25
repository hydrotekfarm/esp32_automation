#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>

// Task handle
TaskHandle_t water_temperature_task_handle;

// Global water temp variable
float _water_temp;

// Water temperature measuring status
bool water_temperature_active;

// Measures water temperature
void measure_water_temperature();
