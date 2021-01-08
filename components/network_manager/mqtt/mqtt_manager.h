#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include <mqtt_client.h>
#include <string.h>

// QOS settings
#define PUBLISH_DATA_QOS 1
#define SUBSCRIBE_DATA_QOS 2

#define WIFI_CONNECT_HEADING "wifi_connect_status"
#define SENSOR_DATA_HEADING "live_data"
#define SENSOR_SETTINGS_HEADING "device_settings"

#define TIME_STRING_LENGTH 21

// Task handle
TaskHandle_t publish_task_handle;

// MQTT client
esp_mqtt_client_handle_t mqtt_client;

// MQTT connect status
bool is_mqtt_connected;

// Topics
char *wifi_connect_topic;
char *sensor_data_topic;
char *sensor_settings_topic;

SemaphoreHandle_t mqtt_connect_semaphore;

// Set broker IP config in MQTT
void mqtt_connect();

// Initialize MQTT connection
void init_mqtt();

// Send mqtt message to publish sensor data to broker
void publish_data();

// Handle data recieved through subscribed topics
void data_handler(char *topic, uint32_t topic_len, char *data, uint32_t data_len);

// Update system settings
void update_settings();

// Create publishing topic
void create_sensor_data_topic();

// Create settings data topic
void create_settings_data_topic();
