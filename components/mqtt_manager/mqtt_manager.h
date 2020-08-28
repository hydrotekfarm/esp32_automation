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

// Topics
char *sensor_data_topic;
char *settings_data_topic;

// Send mqtt message to publish sensor data to broker
void publish_data();

// Handle data recieved through subscribed topics
void data_handler(char *topic, uint32_t topic_len, char *data, uint32_t data_len);

// Create publishing topic
void create_sensor_data_topic();

// Create settings data topic
void create_settings_data_topic();
