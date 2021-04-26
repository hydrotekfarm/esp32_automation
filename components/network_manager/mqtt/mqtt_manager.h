#include <Freertos/freertos.h>
#include <Freertos/task.h>
#include <mqtt_client.h>
#include <cjson.h>
#include <string.h>

#include "rf_transmitter.h"

// QOS settings
#define PUBLISH_DATA_QOS 1
#define SUBSCRIBE_DATA_QOS 2

#define WIFI_CONNECT_HEADING "wifi_connect_status"
#define SENSOR_DATA_HEADING "live_data"
#define SENSOR_SETTINGS_HEADING "device_settings"
#define EQUIPMENT_STATUS_HEADING "equipment_status"
#define GROW_CYCLE_HEADING "device_status"
#define RF_CONTROL_HEADING "manual_rf_control"

#define TIME_STRING_LENGTH 21

#define MQTT_TAG "MQTT_MANAGER"

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
char *equipment_status_topic;
char *grow_cycle_topic;
char *rf_control_topic;

SemaphoreHandle_t mqtt_connect_semaphore;

// JSON objects for equipment status
cJSON *equipment_status_root;
cJSON *control_status_root;
cJSON *ph_control_status;
cJSON *ec_control_status;
cJSON *water_temp_control_status;
cJSON *rf_status_root;
cJSON *rf_statuses[NUM_OUTLETS];


// Get JSON objects
cJSON *get_ph_control_status();
cJSON *get_ec_control_status();
cJSON *get_water_temp_control_status();
cJSON **get_rf_statuses();

// Set broker IP config in MQTT
void mqtt_connect();

// Initialize MQTT connection
void init_mqtt();

// Send mqtt message to publish sensor data to broker
void publish_sensor_data();

// Handle data recieved through subscribed topics
void data_handler(char *topic, uint32_t topic_len, char *data, uint32_t data_len);

// Initialize equipment data JSON
void init_equipment_status();

// Send equipment data over MQTT
void publish_equipment_status();

// Update system settings
void update_settings();

// Create publishing topic
void create_sensor_data_topic();

// Create settings data topic
void create_settings_data_topic();
