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
#include "sync_sensors.h"
#include "rtc.h"
#include "network_settings.h"
#include "wifi_connect.h"

static esp_err_t parse_ota_parameters(const char *buffer, char *device_id, char *version, char *endpoint);
static esp_err_t validate_ota_parameters(char *device_id, char *version, char *endpoint);

extern char *url_buf;
extern bool is_ota_success_on_bootup;

esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
   const char *TAG = "MQTT_Event_Handler";
   esp_mqtt_client_handle_t client = event->client;
   int msg_id;

   switch (event->event_id) {
      case MQTT_EVENT_CONNECTED:
         ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
         xSemaphoreGive(mqtt_connect_semaphore);
         /* Subscribing firmware upgrade topic*/
         msg_id = esp_mqtt_client_subscribe(client, FW_UPGRADE_SUBSCRIBE_TOPIC, 0);
         ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
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

         /* Get FW upgrade URL recevied via MQTT */
         if (NULL != strstr(event->topic, FW_UPGRADE_SUBSCRIBE_TOPIC)) {
            char device_id[6], version[6], endpoint[128];
            if (ESP_OK == parse_ota_parameters(event->data, device_id, version, endpoint)) {
               if (ESP_OK == validate_ota_parameters(device_id, version, endpoint)) {
                  ESP_LOGI(TAG, "FW upgrade command received over MQTT - checking for valid URL\n");
                  if (strlen(endpoint) > OTA_URL_SIZE) {
                     ESP_LOGI(TAG, "URL length is more than valid limit of: [%d]\r\n", OTA_URL_SIZE);
                     publish_ota_result(client, OTA_FAIL, INVALID_OTA_URL_RECEIVED);
                     return ESP_FAIL;
                  }
                  else {
                     /* Copy FW upgrade URL to local buffer */
                     printf("Received URL lenght is: %d\r\n", strlen(endpoint));
                     url_buf = (char *)malloc(strlen(endpoint) + 1);
                     if (NULL == url_buf) {
                        printf("Unable to allocate memory to save received URL\r\n");
                        publish_ota_result(client, OTA_FAIL, INVALID_OTA_URL_RECEIVED);
                     }
                     else {
                        memset(url_buf, 0x00, strlen(endpoint) + 1);
                        strncpy(url_buf, endpoint, strlen(endpoint));
                        printf("Received URL is: %s\r\n", url_buf);

                        /* Starting OTA thread */
                        xTaskCreate(&ota_task, "ota_task", 8192, client, 5, NULL);
                     }
                  }
               }
            }
         }
         else {
            printf("Received topic is not %s\n", FW_UPGRADE_SUBSCRIBE_TOPIC);
            data_handler(event->topic, event->topic_len, event->data, event->data_len);
         }
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

void init_topic(char **topic, int topic_len) {
	*topic = malloc(sizeof(char) * topic_len);
	strcpy(*topic, get_network_settings()->device_id);
}

void add_heading(char *topic, char *heading) {
	strcat(topic, "/");
	strcat(topic, heading);
}

void make_topics() {
	ESP_LOGI("", "Starting make topics");

	int device_id_len = strlen(get_network_settings()->device_id);

	init_topic(&wifi_connect_topic, device_id_len + 1 + strlen(WIFI_CONNECT_HEADING) + 1);
	add_heading(wifi_connect_topic, WIFI_CONNECT_HEADING);
	ESP_LOGI("", "Wifi Topic: %s", wifi_connect_topic);

	init_topic(&sensor_data_topic, device_id_len + 1 + strlen(SENSOR_DATA_HEADING) + 1);
	add_heading(sensor_data_topic, SENSOR_DATA_HEADING);
	ESP_LOGI("", "Sensor data topic: %s", sensor_data_topic);

	init_topic(&sensor_settings_topic, device_id_len + 1 + strlen(SENSOR_SETTINGS_HEADING) + 1);
	add_heading(sensor_settings_topic, SENSOR_SETTINGS_HEADING);
	ESP_LOGI("", "Sensor settings topic: %s", sensor_settings_topic);
}

void subscribe_topics() {
	// Subscribe to topics
	esp_mqtt_client_subscribe(mqtt_client, sensor_settings_topic, SUBSCRIBE_DATA_QOS);
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

	make_topics();
	// TOOD subscribe to topics here
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
	xSemaphoreTake(mqtt_connect_semaphore, portMAX_DELAY); // TODO add approximate time to connect to mqtt

	// Subscribe to topics
	subscribe_topics();

	// Send connect success message (must be retain message)
	esp_mqtt_client_publish(mqtt_client, wifi_connect_topic, "1", 0, PUBLISH_DATA_QOS, 1);

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
	sprintf(time_str + 5, "%.2d", time.tm_mon);
	strcat(time_str, "-");
	sprintf(time_str + 8, "%.2d", time.tm_mday);
	strcat(time_str, "T");
	sprintf(time_str + 11, "%.2d", time.tm_hour);
	strcat(time_str, "-");
	sprintf(time_str + 14, "%.2d", time.tm_min);
	strcat(time_str, "-");
	sprintf(time_str + 17, "%.2d", time.tm_sec);
	strcat(time_str, "Z");

	*time_json = cJSON_CreateString(time_str);
}

void publish_data(void *parameter) {			// MQTT Setup and Data Publishing Task
	const char *TAG = "Publisher";

	ESP_LOGI(TAG, "Sensor data topic: %s", sensor_data_topic);

	for (;;) {
		if(!is_mqtt_connected) {
			ESP_LOGE(TAG, "Wifi not connected, cannot send MQTT data");

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

		ESP_LOGI(TAG, "Message: %s", data);
		ESP_LOGI(TAG, "Topic: %s", sensor_data_topic);

		// Publish data every sensor reading
		vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}

	free(wifi_connect_topic);
	free(sensor_data_topic);
	free(sensor_settings_topic);
}

void update_settings(char *settings) {
	const char *TAG = "UPDATE_SETTINGS";
	ESP_LOGI(TAG, "Settings data");

	cJSON *root = cJSON_Parse(settings);
	cJSON *arr = root->child;

	for(int i = 0; i < cJSON_GetArraySize(arr); i++) {
		cJSON *subitem = cJSON_GetArrayItem(arr, i)->child;
		char *data_topic = subitem->string;

		if(strcmp("ph", data_topic) == 0) {
			ESP_LOGI(TAG, "pH data received");
			ph_update_settings(subitem);
		} else if(strcmp("ec", data_topic) == 0) {
			ESP_LOGI(TAG, "ec data received");
			ec_update_settings(subitem);
		} else if(strcmp("air_temperature", data_topic) == 0) {
			// Add air temp call when control is implemented
			ESP_LOGI(TAG, "air temperature data received");
		} else {
			ESP_LOGE(TAG, "Data %s not recognized", data_topic);
		}
	}

	cJSON_Delete(root);
}

void data_handler(char *topic_in, uint32_t topic_len, char *data_in, uint32_t data_len) {
	const char *TAG = "DATA_HANDLER";

	// Create dynamically allocated vars for topic and data
	char *topic = malloc(sizeof(char) * (topic_len+1));
	char *data = malloc(sizeof(char) * (data_len+1));

	// Copy over topic
	for(int i = 0; i < topic_len; i++) {
		topic[i] = topic_in[i];
	}
	topic[topic_len] = 0;

	// Copy over data
	for(int i = 0; i < data_len; i++) {
		data[i] = data_in[i];
	}
	data[data_len] = 0;

	ESP_LOGI(TAG, "Incoming Topic: %s", topic);

	// Check topic against each subscribed topic possible
	if(strcmp(topic, sensor_settings_topic) == 0) {
		ESP_LOGI(TAG, "Sensor settings received");

		// Update sensor settings
		update_settings(data);
	} else {
		// Topic doesn't match any known topics
		ESP_LOGE(TAG, "Topic unknown");
	}

	free(topic);
	free(data);
}

static esp_err_t validate_ota_parameters(char *device_id, char *version, char *endpoint)
{
   const char *TAG = "VALIDATE_OTA_PARAMETERS";

   ESP_LOGI(TAG, "device_id: \"%s\"\n", device_id);
   ESP_LOGI(TAG, "version: \"%s\"\n", version);
   ESP_LOGI(TAG, "endpoint: \"%s\"\n", endpoint);

   if (device_id == NULL || version == NULL || endpoint == NULL) {
      ESP_LOGI(TAG, "Invalid parameter received");
      return ESP_FAIL;
   }

   int device_id_len = strlen(get_network_settings()->device_id);

   if (!strncmp(get_network_settings()->device_id, device_id, device_id_len)) {
      ESP_LOGI(TAG, "Valid Device ID found: %s %s", get_network_settings()->device_id, device_id);
   } else {
      ESP_LOGI(TAG, "Invalid Device ID found: %s %s", get_network_settings()->device_id, device_id);
      return ESP_FAIL;
   }

   ESP_LOGI(TAG, "FW upgrade command received over MQTT - checking for valid URL\n");
   if (strlen(endpoint) > OTA_URL_SIZE) {
      ESP_LOGI(TAG, "URL length is more than valid limit of: [%d]\r\n", OTA_URL_SIZE);
      return ESP_FAIL;
   }

   return ESP_OK;
}

static esp_err_t parse_ota_parameters(const char *buffer, char *device_id_buf, char *version_buf, char *endpoint_buf)
{
   const char *TAG = "PARSE_OTA_PARAMETERS";

   cJSON *device_id;
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

   device_id = cJSON_GetObjectItemCaseSensitive(root, "device_id");
   if (cJSON_IsString(device_id) && (device_id->valuestring != NULL)) {
      strcpy(device_id_buf, device_id->valuestring);
      ESP_LOGI(TAG, "device_id: \"%s\"\n", device_id_buf);
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

   esp_mqtt_client_publish(client, FW_UPGRADE_PUBLISH_ACK_TOPIC, data, 0, 1, 0);

   ESP_LOGI(TAG, "ota_failed message publish successful, Message: %s", data);
}

void publish_ota_result(esp_mqtt_client_handle_t client, ota_result_t ota_result, ota_failure_reason_t ota_failure_reason) {
   create_and_publish_ota_result(client, ota_result, ota_failure_reason);
}

