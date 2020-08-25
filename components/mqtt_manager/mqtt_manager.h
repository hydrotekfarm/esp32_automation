#include <Freertos/freertos.h>
#include <Freertos/task.h>

// Task handle
TaskHandle_t publish_task_handle;

// IDs
char *growroom_id;
char *system_id;

// Send mqtt message to publish sensor data to broker
void publish_data();
