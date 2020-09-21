#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>

// Task handle
TaskHandle_t ph_task_handle;

// Global ph variable
float _ph;

// Calibration status
bool ph_calibration;

// Measures water ph
void measure_ph();
