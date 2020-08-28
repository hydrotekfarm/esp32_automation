#include <Freertos/freertos.h>
#include <Freertos/task.h>

// QOS settings
#define PUBLISH_DATA_QOS 1
#define SUBSCRIBE_DATA_QOS 2

// Task handle
TaskHandle_t publish_task_handle;

// IDs
char *growroom_id;
char *system_id;

// Send mqtt message to publish sensor data to broker
void publish_data();
