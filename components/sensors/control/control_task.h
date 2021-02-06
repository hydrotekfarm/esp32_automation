#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Task handle
TaskHandle_t sensor_control_task_handle;

// Sensor control task
void sensor_control();

// Reset sensor checks array
void reset_sensor_checks();
