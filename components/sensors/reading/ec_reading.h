#include <stdbool.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>

// Task handle
TaskHandle_t ec_task_handle;

// Global ec variable
float _ec;

// EC measuring status
bool ec_active;

// Calibration status
bool ec_calibration;

// Measures water ph
void measure_ec();
