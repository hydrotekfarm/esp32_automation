#include <stdbool.h>
#include <inttypes.h>
#include <Freertos/freertos.h>
#include <Freertos/task.h>

// Task handle
TaskHandle_t ultrasonic_task_handle;

// Max measuring distance
#define MAX_DISTANCE_CM 500

// Global distance variable
float _distance;

// Distance measuring status
bool ultrasonic_active;

// Measures water level distance
void measure_distance();
