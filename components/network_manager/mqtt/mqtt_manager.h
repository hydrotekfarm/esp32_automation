#ifndef __MQTT_MANAGER_H
#define __MQTT_MANAGER_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mqtt_client.h>
#include <cjson.h>
#include <string.h>
#include <driver/gpio.h>

#include "rf_transmitter.h"

#include "ota.h"

// QOS settings
#define PUBLISH_DATA_QOS 1
#define SUBSCRIBE_DATA_QOS 2

#define DEVICE_TYPE "fertigation"

#define WIFI_CONNECT_HEADING "wifi_connect_status"
#define SENSOR_DATA_HEADING "live_data"
#define SENSOR_SETTINGS_HEADING "device_settings"
#define EQUIPMENT_STATUS_HEADING "equipment_status"
#define GROW_CYCLE_HEADING "device_status"
#define RF_CONTROL_HEADING "manual_rf_control"
#define CALIBRATION_HEADING "calibration"
#define OTA_UPDATE_HEADING "ota_update"
#define OTA_DONE_HEADING "ota_done"
#define VERSION_REQUEST_HEADING "version_request"
#define VERSION_RESULT_HEADING "version_result"
#define TEST_MOTOR_REQUEST_HEADING "test_motor_request"
#define TEST_MOTOR_RESPONSE_HEADING "test_motor_response"
#define TEST_OUTLET_REQUEST_HEADING "test_outlet_request"
#define TEST_OUTLET_RESPONSE_HEADING "test_outlet_response"
#define TEST_SENSOR_REQUEST_HEADING "test_sensor_request"
#define TEST_SENSOR_RESPONSE_HEADING "test_sensor_response"
#define TEST_FS_REQUEST_HEADING "test_fs_request"
#define TEST_FS_RESPONSE_HEADING "test_fs_response"
#define TEST_RF_HEADING "test_rf"

#define DEVICE_ON 1
#define DEVICE_OFF 0
#define DEVICE_ERROR -1

/**
 * OTA Result
 */
typedef enum
{
    OTA_SUCCESS = 0, //!< No alarms
    OTA_FAIL         //!< First alarm
} ota_result_t;

/**
 * OTA Failure Reason
 */
typedef enum
{
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
char *ota_update_topic;
char *ota_done_topic;
char *version_request_topic;
char *version_result_topic;
char *equipment_status_topic;
char *grow_cycle_topic;
char *rf_control_topic;
char *calibration_topic;
char *test_motor_request;
char *test_motor_response;
char *test_outlet_request;
char *test_outlet_response;
char *test_sensor_request;
char *test_sensor_response;
char *test_rf_topic;
char *test_fs_request;
char *test_fs_response;

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

// OTA result publish message
void publish_ota_result(esp_mqtt_client_handle_t client, ota_result_t ota_result, ota_failure_reason_t ota_failure_reason);

// Update calibration settings
void update_calibration(cJSON *obj);

// Publish status for motors
void publish_pump_status(int publish_motor_choice, int publish_status);

// Publish status for power outlet
void publish_power_outlet_status(int outlet_choice, int outlet_status);

// Publish status for float switches
void publish_float_switch_status(int float_switch_choice, int float_switch_status);

// Publish status for sensor
void publish_sensor_status(int sensor_choice, int sensor_status);
#endif
