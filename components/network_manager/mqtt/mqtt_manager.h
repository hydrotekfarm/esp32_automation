#ifndef __MQTT_MANAGER_H
#define __MQTT_MANAGER_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mqtt_client.h>
#include <string.h>

#include "ota.h"

// QOS settings
#define PUBLISH_DATA_QOS 1
#define SUBSCRIBE_DATA_QOS 2

#define WIFI_CONNECT_HEADING "wifi_connect_status"
#define SENSOR_DATA_HEADING "live_data"
#define SENSOR_SETTINGS_HEADING "device_settings"

#define FW_UPGRADE_SUBSCRIBE_TOPIC   "fertigation_ota_push"
#define FW_UPGRADE_PUBLISH_ACK_TOPIC "fertigation_ota_result"

/**
 * OTA Result
 */
typedef enum {
    OTA_SUCCESS   = 0,//!< No alarms
    OTA_FAIL          //!< First alarm
} ota_result_t;

/**
 * OTA Failure Reason
 */
typedef enum {
    VERSION_NOT_FOUND = 0,
    INVALID_OTA_URL_RECEIVED,
    HTTP_CONNECTION_FAILED,
    OTA_UPDATE_FAILED,
    IMAGE_FILE_LARGER_THAN_OTA_PARTITION,
    OTA_WRTIE_OPERATION_FAILED,
    IMAGE_VALIDATION_FAILED,
    OTA_SET_BOOT_PARTITION_FAILED,
    NO_FALIURE
} ota_failure_reason_t;


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

// OTA result publish message
void publish_ota_result(esp_mqtt_client_handle_t client, ota_result_t ota_result, ota_failure_reason_t ota_failure_reason);

#endif
