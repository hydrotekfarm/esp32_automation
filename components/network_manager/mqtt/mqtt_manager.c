#include "mqtt_manager.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_system.h>
#include <freertos/semphr.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cJSON.h>

#include "boot.h"
#include "ec_reading.h"
#include "ph_reading.h"
#include "water_temp_reading.h"
#include "ec_control.h"
#include "ph_control.h"
#include "water_temp_control.h"
#include "sync_sensors.h"
#include "rf_transmitter.h"
#include "rtc.h"
#include "network_settings.h"
#include "grow_manager.h"
#include "wifi_connect.h"
#include "reservoir_control.h"
#include "ports.h"
#include "test_hardware.h"

#define DEVICE_ON 1
#define DEVICE_OFF 0
#define DEVICE_ERROR -1
#define FLOAT_UP 1
#define FLOAT_DOWN 0

static void initiate_ota(const char *mqtt_data);
static esp_err_t parse_ota_parameters(const char *buffer, char *version, char *endpoint);
static esp_err_t validate_ota_parameters(char *version, char *endpoint);
static void publish_firmware_version();
    

extern char *url_buf;
extern bool is_ota_success_on_bootup;

esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
   const char *TAG = "MQTT_Event_Handler";

   switch (event->event_id) {
      case MQTT_EVENT_CONNECTED:
         ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
         xSemaphoreGive(mqtt_connect_semaphore);
         break;
      case MQTT_EVENT_DISCONNECTED:
         ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
         break;

      case MQTT_EVENT_SUBSCRIBED:
         ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
         break;
      case MQTT_EVENT_UNSUBSCRIBED:
         ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
         break;
      case MQTT_EVENT_PUBLISHED:
         ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
         break;
      case MQTT_EVENT_DATA:
         ESP_LOGI(TAG, "MQTT_EVENT_DATA");
         printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
         printf("DATA=%.*s\r\n", event->data_len, event->data);
         data_handler(event->topic, event->topic_len, event->data, event->data_len);
         break;
      case MQTT_EVENT_ERROR:
         ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
         break;
      case MQTT_EVENT_BEFORE_CONNECT:
         ESP_LOGI(TAG, "Before Connection\n");
         break;
      default:
         ESP_LOGI(TAG, "Other event id:%d", event->event_id);
         break;
   }

   return 0;
}

void create_str(char** str, char* init_str) { // Create method to allocate memory and assign initial value to string
	*str = malloc(strlen(init_str) * sizeof(char)); // Assign memory based on size of initial value
	if(!(*str)) { // Restart if memory alloc fails
		ESP_LOGE("", "Memory allocation failed. Restarting ESP32");
		restart_esp32();
	}
	strcpy(*str, init_str); // Copy initial value into string
}
void append_str(char** str, char* str_to_add) { // Create method to reallocate and append string to already allocated string
	*str = realloc(*str, (strlen(*str) + strlen(str_to_add)) * sizeof(char) + 1); // Reallocate data based on existing and new string size
	if(!(*str)) { // Restart if memory alloc fails
		ESP_LOGE("", "Memory allocation failed. Restarting ESP32");
		restart_esp32();
	}
	strcat(*str, str_to_add); // Concatenate strings
}

// Add sensor data to JSON entry
void add_entry(char** data, bool* first, char* name, float num) {
	// Add a comma to the beginning of every entry except the first
	if(*first) *first = false;
	else append_str(data, ",");

	// Convert float data into string
	char value[8];
	snprintf(value, sizeof(value), "%.2f", num);

	// Create entry string
	char *entry = NULL;
	create_str(&entry, "{ \"name\": \"");

	// Create entry using key, value, and other JSON syntax
	append_str(&entry, name);
	append_str(&entry, "\", \"value\": \"");
	append_str(&entry, value);
	append_str(&entry, "\"}");

	// Add entry to overall JSON data
	append_str(data, entry);

	// Free allocated memory
	free(entry);
	entry = NULL;
}

void init_topic(char **topic, int topic_len, char *heading) {
	*topic = malloc(sizeof(char) * topic_len);
	strcpy(*topic, heading);
}

void add_id(char *topic) {
	strcat(topic, "/");
	strcat(topic, get_network_settings()->device_id);
}

void add_device_type(char *topic) {
   strcat(topic, "/");
   strcat(topic, DEVICE_TYPE);
}

void make_topics() {
	ESP_LOGI(MQTT_TAG, "Starting make topics");

	int device_id_len = strlen(get_network_settings()->device_id);
   int device_type_len = strlen(DEVICE_TYPE);
   
	init_topic(&wifi_connect_topic, device_id_len + 1 + strlen(WIFI_CONNECT_HEADING) + 1, WIFI_CONNECT_HEADING);
	add_id(wifi_connect_topic);
	ESP_LOGI(MQTT_TAG, "Wifi Topic: %s", wifi_connect_topic);

	init_topic(&sensor_data_topic, device_id_len + 1 + strlen(SENSOR_DATA_HEADING) + 1, SENSOR_DATA_HEADING);
	add_id(sensor_data_topic);
	ESP_LOGI(MQTT_TAG, "Sensor data topic: %s", sensor_data_topic);

	init_topic(&sensor_settings_topic, device_id_len + 1 + strlen(SENSOR_SETTINGS_HEADING) + 1, SENSOR_SETTINGS_HEADING);
	add_id(sensor_settings_topic);
	ESP_LOGI(MQTT_TAG, "Sensor settings topic: %s", sensor_settings_topic);

	init_topic(&equipment_status_topic, device_id_len + 1 + strlen(EQUIPMENT_STATUS_HEADING) + 1, EQUIPMENT_STATUS_HEADING);
	add_id(equipment_status_topic);
	ESP_LOGI(MQTT_TAG, "Equipment settings topic: %s", equipment_status_topic);

	init_topic(&grow_cycle_topic, device_id_len + 1 + strlen(GROW_CYCLE_HEADING) + 1, GROW_CYCLE_HEADING);
	add_id(grow_cycle_topic);
	ESP_LOGI(MQTT_TAG, "Grow cycle topic: %s", grow_cycle_topic);

	init_topic(&rf_control_topic, device_id_len + 1 + strlen(RF_CONTROL_HEADING) + 1, RF_CONTROL_HEADING);
	add_id(rf_control_topic);
	ESP_LOGI(MQTT_TAG, "RF control settings topic: %s", rf_control_topic);

	//Topic for Calibration//   
   init_topic(&calibration_topic, device_id_len + 1 + strlen(CALIBRATION_HEADING) + 1, CALIBRATION_HEADING);
   add_id(calibration_topic);
   ESP_LOGI(MQTT_TAG, "Calibration sensors topic: %s", calibration_topic);

   init_topic(&test_motor_request, device_id_len + 1 + strlen(TEST_MOTOR_HEADING) + 1, TEST_MOTOR_HEADING);
   add_id(test_motor_request);
   ESP_LOGI(MQTT_TAG, "Test motor request topic: %s", test_motor_request);
   
   init_topic(&test_motor_response, device_id_len + 1 + strlen(TEST_MOTOR_HEADING) + 1, TEST_MOTOR_HEADING);
   add_id(test_motor_response);

   ESP_LOGI(MQTT_TAG, "Test motor response topic: %s", test_motor_response);

   init_topic(&test_outlet_request, device_id_len + 1 + strlen(TEST_OUTLET_HEADING) + 1, TEST_OUTLET_HEADING);
   add_id(test_outlet_request);
   ESP_LOGI(MQTT_TAG, "Test outlet request topic: %s", test_outlet_request);
   
   init_topic(&test_outlet_response, device_id_len + 1 + strlen(TEST_OUTLET_HEADING) + 1, TEST_OUTLET_HEADING);
   add_id(test_outlet_response);
   ESP_LOGI(MQTT_TAG, "Test outlet response topic: %s", test_outlet_response); 
  
   init_topic(&test_sensor_request, device_id_len + 1 + strlen(TEST_SENSOR_HEADING) + 1, TEST_SENSOR_HEADING);
   add_id(test_sensor_request);
   ESP_LOGI(MQTT_TAG, "Test sensor request topic: %s", test_sensor_request);
   
   init_topic(&test_sensor_response, device_id_len + 1 + strlen(TEST_SENSOR_HEADING) + 1, TEST_SENSOR_HEADING);
   add_id(test_sensor_response);
   ESP_LOGI(MQTT_TAG, "Test sensor response topic: %s", test_sensor_response); 

   init_topic(&test_rf_topic, device_id_len + 1 + strlen(TEST_RF_HEADING) + 1, TEST_RF_HEADING);
   add_id(test_rf_topic);
   ESP_LOGI(MQTT_TAG, "Test rf topic: %s", test_rf_topic);

   init_topic(&test_fs_request, device_id_len + 1 + strlen(TEST_FLOAT_SWITCH_HEADING) + 1, TEST_FLOAT_SWITCH_HEADING);
   add_id(test_fs_request);
   ESP_LOGI(MQTT_TAG, "Test float switch request topic: %s",test_fs_request);

   init_topic(&test_fs_response, device_id_len + 1 + strlen(TEST_FLOAT_SWITCH_HEADING) + 1, TEST_FLOAT_SWITCH_HEADING);
   add_id(test_fs_response);
   ESP_LOGI(MQTT_TAG, "Test float switch response topic: %s",test_fs_response);

   init_topic(&ota_update_topic, device_type_len + 1 + strlen(OTA_UPDATE_HEADING) + 1, OTA_UPDATE_HEADING);
   add_device_type(ota_update_topic);
   ESP_LOGI(MQTT_TAG, "OTA update topic: %s", ota_update_topic);

   init_topic(&ota_done_topic, device_type_len + 1 + strlen(OTA_DONE_HEADING) + 1, OTA_DONE_HEADING);
   add_device_type(ota_done_topic);
   ESP_LOGI(MQTT_TAG, "OTA done topic: %s", ota_done_topic);

   init_topic(&version_request_topic, device_type_len + 1 + strlen(VERSION_REQUEST_HEADING) + 1, VERSION_REQUEST_HEADING);
   add_device_type(version_request_topic);
   ESP_LOGI(MQTT_TAG, "Version request topic: %s", version_request_topic);

   init_topic(&version_result_topic, device_type_len + 1 + strlen(VERSION_RESULT_HEADING) + 1, VERSION_RESULT_HEADING);
   add_device_type(version_result_topic);
   ESP_LOGI(MQTT_TAG, "Version result topic: %s", version_result_topic);
}

void subscribe_topics() {
	// Subscribe to topics
	esp_mqtt_client_subscribe(mqtt_client, sensor_settings_topic, SUBSCRIBE_DATA_QOS);
	esp_mqtt_client_subscribe(mqtt_client, grow_cycle_topic, SUBSCRIBE_DATA_QOS);
	esp_mqtt_client_subscribe(mqtt_client, rf_control_topic, SUBSCRIBE_DATA_QOS);
	esp_mqtt_client_subscribe(mqtt_client, calibration_topic, SUBSCRIBE_DATA_QOS);
   esp_mqtt_client_subscribe(mqtt_client, ota_update_topic, SUBSCRIBE_DATA_QOS);
   esp_mqtt_client_subscribe(mqtt_client, version_request_topic, SUBSCRIBE_DATA_QOS);
   esp_mqtt_client_subscribe(mqtt_client, test_motor_response, SUBSCRIBE_DATA_QOS);
   esp_mqtt_client_subscribe(mqtt_client, test_outlet_response, SUBSCRIBE_DATA_QOS);
   esp_mqtt_client_subscribe(mqtt_client, test_sensor_response, SUBSCRIBE_DATA_QOS);
   esp_mqtt_client_subscribe(mqtt_client, test_fs_response, SUBSCRIBE_DATA_QOS);
   esp_mqtt_client_subscribe(mqtt_client, test_rf_topic, SUBSCRIBE_DATA_QOS);
}

void init_mqtt() {
	// Set broker configuration
	esp_mqtt_client_config_t mqtt_cfg = {
			.host = get_network_settings()->broker_ip,
			.port = 1883,
			.event_handle = mqtt_event_handler
	};

	// Create MQTT client
	mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

	// Dynamically create topics
	make_topics();

	// Create equipment status JSON
	init_equipment_status();
}

void mqtt_connect() {
	// First check if wifi is connected
	if(!is_wifi_connected) {
		is_mqtt_connected = false;
		return;
	}

	// Connect mqtt
	mqtt_connect_semaphore = xSemaphoreCreateBinary();
	esp_mqtt_client_start(mqtt_client);
	xSemaphoreTake(mqtt_connect_semaphore, portMAX_DELAY); //  add approximate time to connect to mqtt

	// Subscribe to topics
	subscribe_topics();

	// Send connect success message (must be retain message)
	esp_mqtt_client_publish(mqtt_client, wifi_connect_topic, "1", 0, PUBLISH_DATA_QOS, 1);

	// Send equipment statuses
	publish_equipment_status();

	is_mqtt_connected = true;

   if (is_ota_success_on_bootup == true) {
      printf("Publishing OTA Success result on boot up ...");
      publish_ota_result(mqtt_client, OTA_SUCCESS, NO_FALIURE);
   }
}

void create_time_json(cJSON **time_json) {
	char time_str[TIME_STRING_LENGTH];

	struct tm time;
	get_date_time(&time);

	sprintf(time_str, "%.4d", time.tm_year + 1900);
	strcat(time_str, "-");
	sprintf(time_str + 5, "%.2d", time.tm_mon + 1); // convert from (0 to 11) to (1 to 12)
	strcat(time_str, "-");
	sprintf(time_str + 8, "%.2d", time.tm_mday);
	strcat(time_str, "T");
	sprintf(time_str + 11, "%.2d", time.tm_hour);
	strcat(time_str, ":");
	sprintf(time_str + 14, "%.2d", time.tm_min);
	strcat(time_str, ":");
	sprintf(time_str + 17, "%.2d", time.tm_sec);
	strcat(time_str, "Z");

	*time_json = cJSON_CreateString(time_str);
}

void publish_sensor_data(void *parameter) {			// MQTT Setup and Data Publishing Task
	ESP_LOGI(MQTT_TAG, "Sensor data topic: %s", sensor_data_topic);

	for (;;) {
		if(!is_mqtt_connected) {
			ESP_LOGE(MQTT_TAG, "Wifi not connected, cannot send MQTT data");

			// Wait for delay period and try again
			vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
			continue;
		}

		cJSON *root, *time, *sensor_arr, *sensor;

		// Initializing json object and sensor array
		root = cJSON_CreateObject();
		sensor_arr = cJSON_CreateArray();

		// Adding time
		create_time_json(&time);
		cJSON_AddItemToObject(root, "time", time);

		// Adding water temperature
		sensor_get_json(get_water_temp_sensor(), &sensor);
		cJSON_AddItemToArray(sensor_arr, sensor);

		// Adding ec
		sensor_get_json(get_ec_sensor(), &sensor);
		cJSON_AddItemToArray(sensor_arr, sensor);

		// Adding pH
		sensor_get_json(get_ph_sensor(), &sensor);
		cJSON_AddItemToArray(sensor_arr, sensor);

		// Adding array to object
		cJSON_AddItemToObject(root, "sensors", sensor_arr);

		// Creating string from JSON
		char *data = cJSON_PrintUnformatted(root);

		// Free memory
		cJSON_Delete(root);

		// Publish data to MQTT broker using topic and data
		esp_mqtt_client_publish(mqtt_client, sensor_data_topic, data, 0, PUBLISH_DATA_QOS, 0);

      


		ESP_LOGI(MQTT_TAG, "Sensor data: %s", data);

		// Publish data every sensor reading
		vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}

	free(wifi_connect_topic);
	free(sensor_data_topic);
	free(sensor_settings_topic);
}

cJSON* get_ph_control_status() { return ph_control_status; }
cJSON* get_ec_control_status() { return ec_control_status; }
cJSON* get_water_temp_control_status() { return water_temp_control_status; }
cJSON **get_rf_statuses() { return rf_statuses; }

void init_equipment_status() {
	equipment_status_root = cJSON_CreateObject();
	control_status_root = cJSON_CreateObject();
	rf_status_root = cJSON_CreateObject();

	// Create sensor statuses
	ph_control_status = cJSON_CreateNumber(0);
	ec_control_status = cJSON_CreateNumber(0);
	water_temp_control_status = cJSON_CreateNumber(0);
	cJSON_AddItemToObject(control_status_root, "ph_control", ph_control_status);
	cJSON_AddItemToObject(control_status_root, "ec_control", ec_control_status);
	cJSON_AddItemToObject(control_status_root, "water_temp_control", water_temp_control_status);

	// Create rf statuses
	char key[3];
	for(uint8_t i = 0; i < NUM_OUTLETS; ++i) {
		rf_statuses[i] = cJSON_CreateNumber(0);

		sprintf(key, "%d", i);
		cJSON_AddItemToObject(rf_status_root, key, rf_statuses[i]);
	}

	cJSON_AddItemToObject(equipment_status_root, "rf", rf_status_root);
	cJSON_AddItemToObject(equipment_status_root, "control", control_status_root);
}

void publish_equipment_status() {
	char *data = cJSON_Print(equipment_status_root); // Create data string
	esp_mqtt_client_publish(mqtt_client, equipment_status_topic, data, 0, PUBLISH_DATA_QOS, 1); // Publish data
	ESP_LOGI(MQTT_TAG, "Equipment Data: %s", data);
}

void update_settings(char *settings) {
	cJSON *root = cJSON_Parse(settings);
	char* string = cJSON_Print(root);
	ESP_LOGI(MQTT_TAG, "datavalue:\n %s\n", string);
	cJSON *object_settings = root->child;
	
	char *data_topic = object_settings->string;
	ESP_LOGI(MQTT_TAG, "datatopic: %s\n", data_topic);

	if(strcmp("ph", data_topic) == 0) {
		ESP_LOGI(MQTT_TAG, "pH data received");
		ph_update_settings(object_settings);
	} else if(strcmp("ec", data_topic) == 0) {
		ESP_LOGI(MQTT_TAG, "EC data received");
		ec_update_settings(object_settings);
	} else if(strcmp("water_temp", data_topic) == 0) {
		ESP_LOGI(MQTT_TAG, "Water Temperature data received");
		water_temp_update_settings(object_settings);
	} else if(strcmp("irrigation", data_topic) == 0) {
		ESP_LOGI(MQTT_TAG, "Irrigation data received");
		update_irrigation_timings(object_settings);
	} else if(strcmp("grow_lights", data_topic) == 0) {
		ESP_LOGI(MQTT_TAG, "Grow Lights data received");
		update_grow_light_timings(object_settings);
	} else if(strcmp("reservoir", data_topic) == 0) {
		ESP_LOGI(MQTT_TAG, "Reservoir data received");
		update_reservoir_settings(object_settings);
	} else {
		ESP_LOGE(MQTT_TAG, "Data %s not recognized", data_topic);
	}
	cJSON_Delete(root);

	ESP_LOGI(MQTT_TAG, "Settings updated");
	if(!get_is_settings_received()) settings_received();
}

static void initiate_ota(const char *mqtt_data) {
   const char *TAG = "INITIATE_OTA";

   char version[FIRMWARE_VERSION_LEN], endpoint[OTA_URL_SIZE];
   if (ESP_OK == parse_ota_parameters(mqtt_data, version, endpoint)) {
      if (ESP_OK == validate_ota_parameters(version, endpoint)) {
         ESP_LOGI(TAG, "FW upgrade command received over MQTT - checking for valid URL\n");
         if (strlen(endpoint) > OTA_URL_SIZE) {
            ESP_LOGI(TAG, "URL length is more than valid limit of: [%d]\r\n", OTA_URL_SIZE);
            publish_ota_result(mqtt_client, OTA_FAIL, INVALID_OTA_URL_RECEIVED);
         }
         else {
            /* Copy FW upgrade URL to local buffer */
            printf("Received URL lenght is: %d\r\n", strlen(endpoint));
            url_buf = (char *)malloc(strlen(endpoint) + 1);
            if (NULL == url_buf) {
               printf("Unable to allocate memory to save received URL\r\n");
               publish_ota_result(mqtt_client, OTA_FAIL, INVALID_OTA_URL_RECEIVED);
            }
            else {
               memset(url_buf, 0x00, strlen(endpoint) + 1);
               strncpy(url_buf, endpoint, strlen(endpoint));
               printf("Received URL is: %s\r\n", url_buf);

               /* Starting OTA thread */
               xTaskCreate(&ota_task, "ota_task", 8192, mqtt_client, 5, NULL);
            }
         }
      }
   }
}

static esp_err_t validate_ota_parameters(char *version, char *endpoint)
{
   const char *TAG = "VALIDATE_OTA_PARAMETERS";

   ESP_LOGI(TAG, "version: \"%s\"\n", version);
   ESP_LOGI(TAG, "endpoint: \"%s\"\n", endpoint);

   if (version == NULL || endpoint == NULL) {
      ESP_LOGI(TAG, "Invalid parameter received");
      return ESP_FAIL;
   }

   ESP_LOGI(TAG, "FW upgrade command received over MQTT - checking for valid URL\n");
   if (strlen(endpoint) > OTA_URL_SIZE) {
      ESP_LOGI(TAG, "URL length is more than valid limit of: [%d]\r\n", OTA_URL_SIZE);
      return ESP_FAIL;
   }

   return ESP_OK;
}

static esp_err_t parse_ota_parameters(const char *buffer, char *version_buf, char *endpoint_buf)
{
   const char *TAG = "PARSE_OTA_PARAMETERS";

   cJSON *version;
   cJSON *endpoint;

   if (buffer == NULL) {
      ESP_LOGI(TAG, "Invalid parameter received");
      return ESP_FAIL;
   }

   cJSON *root = cJSON_Parse(buffer);
   if (root == NULL) {
      ESP_LOGI(TAG, "Fail to deserialize Json");
      return ESP_FAIL;
   }

   version = cJSON_GetObjectItemCaseSensitive(root, "version");
   if (cJSON_IsString(version) && (version->valuestring != NULL)) {
      strcpy(version_buf, version->valuestring);
      ESP_LOGI(TAG, "version: \"%s\"\n", version_buf);
   }

   endpoint = cJSON_GetObjectItemCaseSensitive(root, "endpoint");
   if (cJSON_IsString(endpoint) && (endpoint->valuestring != NULL)) {
      strcpy(endpoint_buf, endpoint->valuestring);
      ESP_LOGI(TAG, "endpoint: \"%s\"\n", endpoint_buf);
   }
   return ESP_OK;
}

static void create_and_publish_ota_result(esp_mqtt_client_handle_t client, ota_result_t ota_result, ota_failure_reason_t ota_failure_reason)
{
   const char *TAG = "CREATE_AND_PUBLISH_OTA_RESULT";
   cJSON *root, *device_id, *version, *result, *error;

   // Initializing json object and sensor array
   root = cJSON_CreateObject();

   // Adding Device ID
   device_id = cJSON_CreateString(get_network_settings()->device_id);
   cJSON_AddItemToObject(root, "device_id", device_id);

   // Adding Device version
   esp_app_desc_t running_app_info;
   const esp_partition_t *running = esp_ota_get_running_partition();

   if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
      ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
      version = cJSON_CreateString(running_app_info.version);
      cJSON_AddItemToObject(root, "version", version);
   }

   if (ota_result == OTA_SUCCESS) {
      result = cJSON_CreateString("success");
      cJSON_AddItemToObject(root, "result", result);
      error = cJSON_CreateString("");
      cJSON_AddItemToObject(root, "error", error);
   }
   else {
      result = cJSON_CreateString("failure");
      cJSON_AddItemToObject(root, "result", result);
      if (ota_failure_reason == VERSION_NOT_FOUND) {
         error = cJSON_CreateString("version not found");
      }
      else if (ota_failure_reason == INVALID_OTA_URL_RECEIVED) {
         error = cJSON_CreateString("Invalid OTA URL received");
      }
      else if (ota_failure_reason == HTTP_CONNECTION_FAILED) {
         error = cJSON_CreateString("http connection failed");
      }
      else if (ota_failure_reason == OTA_UPDATE_FAILED) {
         error = cJSON_CreateString("ota update failed");
      }
      else if (ota_failure_reason == IMAGE_FILE_LARGER_THAN_OTA_PARTITION) {
         error = cJSON_CreateString("image file larger than ota partition");
      }
      else if (ota_failure_reason == OTA_WRTIE_OPERATION_FAILED) {
         error = cJSON_CreateString("ota write operation failed");
      }
      else if (ota_failure_reason == IMAGE_VALIDATION_FAILED) {
         error = cJSON_CreateString("version not found");
      }
      else if (ota_failure_reason == OTA_SET_BOOT_PARTITION_FAILED) {
         error = cJSON_CreateString("version not found");
      }
      else{
         error = cJSON_CreateString("version not found");
      }

      cJSON_AddItemToObject(root, "error", error);
   }

   // Creating string from JSON
   char *data = cJSON_PrintUnformatted(root);

   // Free memory
   cJSON_Delete(root);

   ESP_LOGI(TAG, "Message: %s", data);

   esp_mqtt_client_publish(client, ota_done_topic, data, 0, 1, 0);

   ESP_LOGI(TAG, "ota_failed message publish successful, Message: %s", data);
}

void publish_ota_result(esp_mqtt_client_handle_t client, ota_result_t ota_result, ota_failure_reason_t ota_failure_reason) {
   create_and_publish_ota_result(client, ota_result, ota_failure_reason);
}

void data_handler(char *topic_in, uint32_t topic_len, char *data_in, uint32_t data_len)
{
   const char *TAG = "DATA_HANDLER";

   // Create dynamically allocated vars for topic and data
   char *topic = malloc(sizeof(char) * (topic_len + 1));
   char *data = malloc(sizeof(char) * (data_len + 1));

   // Copy over topic
   for (int i = 0; i < topic_len; i++)
   {
      topic[i] = topic_in[i];
   }
   topic[topic_len] = 0;

   // Copy over data
   for (int i = 0; i < data_len; i++)
   {
      data[i] = data_in[i];
   }
   data[data_len] = 0;

   ESP_LOGI(TAG, "Incoming Topic: %s", topic);

   // Check topic against each subscribed topic possible
   if (strcmp(topic, sensor_settings_topic) == 0)
   {
      // Update sensor settings
      ESP_LOGI(TAG, "Sensor settings received");
      update_settings(data);
   }
   else if (strcmp(topic, grow_cycle_topic) == 0)
   {
      // Start/stop grow cycle according to message
      ESP_LOGI(TAG, "Grow cycle status received");
      if (data[0] == '0')
         stop_grow_cycle();
      else
         start_grow_cycle();
   }
   else if (strcmp(topic, rf_control_topic) == 0)
   {
      cJSON *obj = cJSON_Parse(data);
      obj = obj->child;
      ESP_LOGI(TAG, "RF id number %d: RF state: %d", atoi(obj->string), obj->valueint);
      control_power_outlet(atoi(obj->string), obj->valueint);
   }
   else if (strcmp(topic, calibration_topic) == 0)
   {
      cJSON *obj = cJSON_Parse(data);
      update_calibration(obj);
   }
   else if (strcmp(topic, ota_update_topic) == 0)
   {
      // Initiate ota
      ESP_LOGI(TAG, "OTA update message received");
      initiate_ota(data);
   }
   else if (strcmp(topic, version_request_topic) == 0)
   {
      // Send back firmware version
      ESP_LOGI(TAG, "Firmware version requested");
      publish_firmware_version();
   }

   else if (strcmp(topic, test_motor_request) == 0)
   {
      int pump_status = 0;
      cJSON *choice;
      cJSON *switch_status;
      cJSON *root = cJSON_Parse(data);
      choice = cJSON_GetObjectItemCaseSensitive(root, "choice");
      switch_status = cJSON_GetObjectItemCaseSensitive(root, "switch_status");
      if (cJSON_IsNumber(switch_status))
      {
         if (switch_status->valueint == DEVICE_OFF || switch_status->valueint == DEVICE_ON)
         {
            pump_status = switch_status->valueint;
            ESP_LOGI(TAG, "Received the test motor message");
            ESP_LOGI(TAG, "Pump status:%d\n", pump_status);
            test_motor(choice->valueint, pump_status);
         }
         else
         {
            ESP_LOGE(TAG, "Invalid device status:%d\n", switch_status->valueint);
         }
      }
      else
      {
         ESP_LOGE(TAG, "The provided status need to be an integer");
      }
   }
   else if (strcmp(topic, test_outlet_request) == 0)
   {
      int outlet_status = 0;
      cJSON *choice;
      cJSON *switch_status;
      cJSON *root = cJSON_Parse(data);
      choice = cJSON_GetObjectItemCaseSensitive(root, "choice");
      switch_status = cJSON_GetObjectItemCaseSensitive(root, "switch_status");
      if (cJSON_IsNumber(switch_status) && cJSON_IsNumber(choice))
      {
         if (switch_status->valueint == DEVICE_OFF || switch_status->valueint == DEVICE_ON)
         {
            outlet_status = switch_status->valueint;
            ESP_LOGI(TAG, "Received the test outlet message");
            ESP_LOGI(TAG, "Outlet status:%d\n", outlet_status);
            test_outlet(choice->valueint, outlet_status);
         }
         else
         {
            ESP_LOGE(TAG, "Invalid device status:%d\n", switch_status->valueint);
         }
      }
      else
      {
         ESP_LOGE(TAG, "The provided status or choice need to be an integer");
      }
   }

   else if (strcmp(topic, test_sensor_request) == 0)
   {
      int sensor_status = 0;
      cJSON *choice;
      cJSON *switch_status;
      cJSON *root = cJSON_Parse(data);
      choice = cJSON_GetObjectItemCaseSensitive(root, "choice");
      switch_status = cJSON_GetObjectItemCaseSensitive(root, "switch_status");
      if (cJSON_IsNumber(switch_status) && cJSON_IsNumber(choice))
      {
         if (switch_status->valueint == DEVICE_OFF || switch_status->valueint == DEVICE_ON)
         {
            sensor_status = switch_status->valueint;
            ESP_LOGI(TAG, "Received the test sensor message");
            ESP_LOGI(TAG, "Sensor status:%d\n", sensor_status);
            test_sensor(choice->valueint, sensor_status);
         }
         else
         {
            ESP_LOGE(TAG, "Invalid device status:%d\n", switch_status->valueint);
         }
      }
      else
      {
         ESP_LOGE(TAG, "The provided status or choice need to be an integer");
      }
   }

   else if (strcmp(topic, test_fs_request) == 0)
   {
      int float_switch_status = 0;
      cJSON *choice;
      cJSON *switch_status;
      cJSON *obj = cJSON_Parse(data);
      choice = cJSON_GetObjectItemCaseSensitive(obj, "choice");
      switch_status = cJSON_GetObjectItemCaseSensitive(obj, "switch_status");
      if (cJSON_IsNumber(choice) && cJSON_IsNumber(switch_status))
      {
         if (switch_status->valueint == DEVICE_ON || switch_status->valueint == DEVICE_OFF)
         {
            float_switch_status = switch_status->valueint;
            ESP_LOGI(TAG, "status: %d\n", float_switch_status);
            ESP_LOGI(TAG, "Received the test float switch message");
            test_float_switch(choice->valueint, float_switch_status);
         }
         else
         {
            ESP_LOGE(TAG, "Invalid device status:%d\n", switch_status->valueint);
         }
      }
      else
      {
         ESP_LOGE(TAG, "Enter valid integer for choice or status ");
      }
   }
   
   else if (strcmp(topic, test_rf_topic) == 0)
   {
      ESP_LOGI(TAG, "Received the test RF message");
      test_rf();
   }
   else
   {
      // Topic doesn't match any known topics
      ESP_LOGE(TAG, "Topic unknown");
   }

   free(topic);
   free(data);
}

static void publish_firmware_version()
{
   cJSON *root, *device_id, *version;
   root = cJSON_CreateObject();

   // Adding Device ID
   device_id = cJSON_CreateString(get_network_settings()->device_id);
   cJSON_AddItemToObject(root, "device_id", device_id);

   // Adding version
   char firmware_version[FIRMWARE_VERSION_LEN];
   if (get_firmware_version(firmware_version))
   {
      version = cJSON_CreateString(firmware_version);
   }
   else
   {
      version = cJSON_CreateString("error");
   }
   cJSON_AddItemToObject(root, "version", version);

   esp_mqtt_client_publish(mqtt_client, version_result_topic, cJSON_PrintUnformatted(root), 0, 1, 0);
   cJSON_Delete(root);
}

void update_calibration(cJSON *data)
{
   cJSON *obj = data->child;
   char *data_string = cJSON_Print(data);
   ESP_LOGI(MQTT_TAG, "%s", data_string);
   if (strcmp(obj->string, "type") == 0)
   {
      if (strcmp(obj->valuestring, "ph") == 0)
      {
         sensor_set_calib_status(get_ph_sensor(), true);
         ESP_LOGI(MQTT_TAG, "pH calibration received");
         if (!get_is_grow_active())
         {
            vTaskResume(*sensor_get_task_handle(get_water_temp_sensor()));
            vTaskResume(*sensor_get_task_handle(get_ph_sensor()));
            ESP_LOGI(MQTT_TAG, "pH and water_temp task resumed");
         }
      }
      else if (strcmp(obj->valuestring, "ec_wet") == 0)
      {
         sensor_set_calib_status(get_ec_sensor(), true);
         ESP_LOGI(MQTT_TAG, "ec wet calibration received");
         if (!get_is_grow_active())
         {
            vTaskResume(*sensor_get_task_handle(get_ec_sensor()));
            ESP_LOGI(MQTT_TAG, "ec task resumed");
         }
      }
      else if (strcmp(obj->valuestring, "ec_dry") == 0)
      {
         dry_calib = true;
         ESP_LOGI(MQTT_TAG, "ec dry calibration received");
         if (!get_is_grow_active())
         {
            vTaskResume(*sensor_get_task_handle(get_ec_sensor()));
            ESP_LOGI(MQTT_TAG, "ec task resumed");
         }
      }
      else
      {
         ESP_LOGE(MQTT_TAG, "Invalid Value Recieved");
      }
   }
   else
   {
      ESP_LOGE(MQTT_TAG, "Invalid Key Recieved, Expected Key: type");
   }
   cJSON_Delete(data);
}

void publish_pump_status(int publish_motor_choice , int publish_status){
   const char *TAG = "PUBLISH_PUMP_STATUS";
   cJSON *temp_obj, *choice=NULL, *status=NULL;
   temp_obj = cJSON_CreateObject();
   
   choice = cJSON_CreateNumber(publish_motor_choice);
   status = cJSON_CreateNumber(publish_status);

   //ch = cJSON_Create(choice->ch);
   cJSON_AddItemToObject(temp_obj, "Choice", choice);

   //stat = cJSON_CreateString(error_status->stat);
   cJSON_AddItemToObject(temp_obj, "Status", status);

   char *data = cJSON_PrintUnformatted(temp_obj);
   

   ESP_LOGI(TAG, "Message: %s", data);

   esp_mqtt_client_publish(mqtt_client,test_motor_response, data, 0, 1, 0);
   cJSON_Delete(temp_obj);
}

void publish_light_status(int publish_light_choice, int publish_status)
{
   const char *TAG = "PUBLISH_LIGHT_STATUS";
   cJSON *info, *choice=NULL, *status=NULL;
   info = cJSON_CreateObject();
   choice = cJSON_CreateNumber(publish_light_choice);
   status = cJSON_CreateNumber(publish_status);

   cJSON_AddItemToObject(info, "Choice", choice);

   cJSON_AddItemToObject(info, "Status", status);
   char *data = cJSON_PrintUnformatted(info);

   ESP_LOGI(TAG, "Message: %s", data);
   esp_mqtt_client_publish(mqtt_client, test_sensor_response, data, 0, 1, 0);
   cJSON_Delete(info);
}

void publish_water_cooler_status(int publish_cooler_status)
{
   const char *TAG = "PUBLISH_WATER_COOLER";
   cJSON *root, *status=NULL;
   root = cJSON_CreateObject();
   status = cJSON_CreateNumber(publish_cooler_status);

   cJSON_AddItemToObject(root,"Status",status);
   char *data = cJSON_PrintUnformatted(root);

   ESP_LOGI(TAG,"Message: %s", data);
   esp_mqtt_client_publish(mqtt_client, test_outlet_response, data, 0, 1, 0);
   cJSON_Delete(root);
}

void publish_water_heater_status(int publish_heater_status)
{
   const char *TAG = "PUBLISH_WATER_HEATER";
   cJSON *root,*status=NULL;
   root = cJSON_CreateObject();
   status = cJSON_CreateNumber(publish_heater_status);

   cJSON_AddItemToObject(root,"Status",status);
   char *data = cJSON_PrintUnformatted(root);

   ESP_LOGI(TAG,"Message: %s", data);
   esp_mqtt_client_publish(mqtt_client, test_outlet_response, data, 0, 1, 0);
   cJSON_Delete(root);
}

void publish_water_in_status(int publish_in_status)
{
   const char *TAG = "PUBLISH_WATER_IN";
   cJSON *root,*status=NULL;
   root = cJSON_CreateObject();
   status = cJSON_CreateNumber(publish_in_status);

   cJSON_AddItemToObject(root,"Status",status);
   char *data = cJSON_PrintUnformatted(root);

   ESP_LOGI(TAG,"Message: %s", data);
   esp_mqtt_client_publish(mqtt_client, test_outlet_response, data, 0, 1, 0);
   cJSON_Delete(root);
}

void publish_water_out_status(int publish_out_status)
{
   const char *TAG = "PUBLISH_WATER_OUT";
   cJSON *root,*status=NULL;
   root = cJSON_CreateObject();
   status = cJSON_CreateNumber(publish_out_status);

   cJSON_AddItemToObject(root,"Status",status);
   char *data = cJSON_PrintUnformatted(root);

   ESP_LOGI(TAG,"Message: %s", data);
   esp_mqtt_client_publish(mqtt_client, test_outlet_response, data, 0, 1, 0);
   cJSON_Delete(root);
}

void publish_irrigation_status(int publish_irrig_status)
{
   const char *TAG = "PUBLISH_IRRIGATION";
   cJSON *root,*status=NULL;
   root = cJSON_CreateObject();


   cJSON_AddItemToObject(root,"Status",status);
   char *data = cJSON_PrintUnformatted(root);

   ESP_LOGI(TAG,"Message: %s", data);
   esp_mqtt_client_publish(mqtt_client, test_outlet_response, data, 0, 1, 0);
   cJSON_Delete(root);
}

void publish_float_switch_status(int float_switch_type , int float_switch_status){
   const char *TAG = "PUBLISH_FLOAT SWITCH_STATUS";
   cJSON *temp_obj, *type=NULL, *status=NULL;
   temp_obj = cJSON_CreateObject();
   type=cJSON_CreateNumber(float_switch_type);
   status=cJSON_CreateNumber(float_switch_status);
  
   cJSON_AddItemToObject(temp_obj, "Type", type);

   cJSON_AddItemToObject(temp_obj, "Status", status);

   char *data = cJSON_PrintUnformatted(temp_obj);
   

   ESP_LOGI(TAG, "Message: %s", data);

   esp_mqtt_client_publish(mqtt_client,test_fs_response, data, 0, 1, 0);
   cJSON_Delete(temp_obj);
}

void publish_ph_status(int ph_status)
{
   const char *TAG = "PUBLISH_PH";
   cJSON *root,*status=NULL;
   root = cJSON_CreateObject();
   status = cJSON_CreateNumber(ph_status);

   cJSON_AddItemToObject(root,"Status",status);
   char *data = cJSON_PrintUnformatted(root);

   ESP_LOGI(TAG,"Message: %s", data);
   esp_mqtt_client_publish(mqtt_client, test_sensor_response, data, 0, 1, 0);
   cJSON_Delete(root);
}

void publish_ec_status(int ec_status)
{
   const char *TAG = "PUBLISH_EC";
   cJSON *root,*status=NULL;
   root = cJSON_CreateObject();
   status = cJSON_CreateNumber(ec_status);

   cJSON_AddItemToObject(root,"Status", status);
   char *data = cJSON_PrintUnformatted(root);

   ESP_LOGI(TAG,"Message: %s", data);
   esp_mqtt_client_publish(mqtt_client, test_sensor_response, data, 0, 1, 0);
   cJSON_Delete(root);
}

void publish_water_temperature_status(int publish_temperature_status)
{
   const char *TAG = "PUBLISH_WATER_TEMPERATURE";
   cJSON *root,*status=NULL;
   root = cJSON_CreateObject();
   status = cJSON_CreateNumber(publish_temperature_status);

   cJSON_AddItemToObject(root,"Status",status);
   char *data = cJSON_PrintUnformatted(root);

   ESP_LOGI(TAG,"Message: %s", data);
   esp_mqtt_client_publish(mqtt_client, test_sensor_response, data, 0, 1, 0);
   cJSON_Delete(root);
}
