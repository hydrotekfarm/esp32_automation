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

#include "ota.h"

extern char *url_buf;

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
         if (NULL != strstr(event->topic, FW_UPGRADE_SUBSCRIBE_TOPIC))
         {
            printf("FW upgrade command received over MQTT - checking for valid URL\n");
            if (event->data_len > OTA_URL_SIZE)
            {
               printf("URL length is more than valid limit of: [%d]\r\n", OTA_URL_SIZE);
               publish_ota_reject(client);
            }
            else
            {
               /* Copy FW upgrade URL to local buffer */
               printf("Received URL lenght is: %d\r\n", event->data_len);
               url_buf = (char *)malloc(event->data_len + 1);
               if (NULL == url_buf)
               {
                  printf("Unable to allocate memory to save received URL\r\n");
                  publish_ota_reject(client);
               }
               else
               {
                  memset(url_buf, 0x00, event->data_len + 1);
                  strncpy(url_buf, event->data, event->data_len);
                  printf("Received URL is: %s\r\n", url_buf);

                  /* Starting OTA thread */
                  xTaskCreate(&ota_task, "ota_task", 8192, client, 5, NULL);
               }
            }
         }
         else
         {
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

void publish_ota_reject(esp_mqtt_client_handle_t client)
{
   int msg_id;
   uint8_t mac[6];//To save WiFi STA MAC address
   char mac_str[18];
   char msg[32];
   const char *TAG = "PUBLISH_OTA_REJECT";
   printf("Aborting OTA\r\n");
   esp_read_mac(mac, ESP_MAC_WIFI_STA);
   sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
   snprintf(msg, sizeof(msg), "%s:ota_failed", mac_str);
   ESP_LOGI(TAG, "msg=%s", msg);
   msg_id = esp_mqtt_client_publish(client, FW_UPGRADE_PUBLISH_ACK_TOPIC, msg, 0, 1, 0);
   ESP_LOGI(TAG, "ota_failed message publish successful, msg_id=%d", msg_id);
}

