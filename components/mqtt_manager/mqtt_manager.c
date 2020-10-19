#include "mqtt_manager.h"

#include <mqtt_client.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/semphr.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cjson.h>

#include "boot.h"
#include "ec_reading.h"
#include "ph_reading.h"
#include "ultrasonic_reading.h"
#include "water_temp_reading.h"
#include "ec_control.h"
#include "ph_control.h"
#include "sync_sensors.h"
#include "rtc.h"

static void mqtt_event_handler(esp_mqtt_event_handle_t event) {		// MQTT Event Callback Functions
	const char *TAG = "MQTT_Event_Handler";
	switch (event->event_id) {
	case MQTT_EVENT_CONNECTED:
		xTaskNotifyGive(publish_task_handle);
		ESP_LOGI(TAG, "Connected\n");
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "Disconnected\n");
		break;
	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "Subscribed\n");
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "UnSubscribed\n");
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "Published\n");
		break;
	case MQTT_EVENT_DATA:
		ESP_LOGI(TAG, "Message received\n");
		data_handler(event->topic, event->topic_len, event->data, event->data_len);
		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "Error\n");
		break;
	case MQTT_EVENT_BEFORE_CONNECT:
		ESP_LOGI(TAG, "Before Connection\n");
		break;
	default:
		ESP_LOGI(TAG, "Other Command\n");
		break;
	}
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
	create_str(&entry, "{ name: \"");

	// Create entry using key, value, and other JSON syntax
	append_str(&entry, name);
	append_str(&entry, "\", value: \"");
	append_str(&entry, value);
	append_str(&entry, "\"}");

	// Add entry to overall JSON data
	append_str(data, entry);

	// Free allocated memory
	free(entry);
	entry = NULL;
}

void publish_data(void *parameter) {			// MQTT Setup and Data Publishing Task
	const char *TAG = "Publisher";

	cluster_id = "GrowRoom1";
	device_id = "System1";

	// Set broker configuration
	esp_mqtt_client_config_t mqtt_cfg = {
			.host = "136.37.190.205",
			.port = 1883,
			.event_handle = mqtt_event_handler
	};

	// Create and initialize MQTT client
	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client);
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	// Create topics
	create_sensor_data_topic();
	create_settings_data_topic();

	ESP_LOGI(TAG, "Sensor data topic4: %s", sensor_data_topic);

	// Subscribe to topics
	esp_mqtt_client_subscribe(client, settings_data_topic, SUBSCRIBE_DATA_QOS);

	for (;;) {
		// Create and initially assign JSON data
		char *data = NULL;
		create_str(&data, "{ \"time\": ");

		// Add timestamp to data
		struct tm time;
		get_date_time(&time);

		// Convert all time componenets to string
		uint32_t year_int = time.tm_year + 1900;
		char year[8];
		snprintf(year, sizeof(year), "%.4d", year_int);

		uint32_t month_int = time.tm_mon + 1;
		char month[8];
		snprintf(month, sizeof(month), "%.2d", month_int);

		char day[8];
		snprintf(day, sizeof(day), "%.2d", time.tm_mday);

		char hour[8];
		snprintf(hour, sizeof(hour), "%.2d", time.tm_hour);

		char min[8];
		snprintf(min, sizeof(min), "%.2d", time.tm_min);

		char sec[8];
		snprintf(sec, sizeof(sec), "%.2d", time.tm_sec);

		// Format timestamp in standard ISO format (https://www.w3.org/TR/NOTE-datetime)
		char *date = NULL;;
		create_str(&date, "\"");
		append_str(&date, year);
		append_str(&date, "-");
		append_str(&date, month);
		append_str(&date, "-");
		append_str(&date,  day);
		append_str(&date, "T");
		append_str(&date, hour);
		append_str(&date, "-");
		append_str(&date, min);
		append_str(&date, "-");
		append_str(&date, sec);
		append_str(&date, "Z\", sensors: [");

		// Append formatted timestamp to data
		append_str(&data, date);
		free(date);
		date = NULL;

		// Variable for adding comma to every entry except first
		bool first = true;

		// Check if all the sensors are active and add data to JSON string if so using corresponding key and value
		add_entry(&data, &first, "water temp", _water_temp);
		add_entry(&data, &first, "ec", _ec);
		add_entry(&data, &first, "ph", _ph);
		if(ultrasonic_active) { add_entry(&data, &first, "distance", _distance); }

		// Add closing tag
		append_str(&data, "]}");

		// Publish data to MQTT broker using topic and data
		esp_mqtt_client_publish(client, sensor_data_topic, data, 0, PUBLISH_DATA_QOS, 0);

		ESP_LOGI(TAG, "Message: %s", data);
		ESP_LOGI(TAG, "Topic: %s", sensor_data_topic);

		// Free allocated memory
		free(data);
		data = NULL;

		// Publish data every sensor reading
		vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}

void update_settings() {
	const char *TAG = "UPDATE_SETTINGS";
	ESP_LOGI(TAG, "Settings data");

	char *data_string = "{\"data\":[{\"ph\":{\"monitoring_only\":false,\"control\":{\"ph_up_down\":null,\"dosing_time\":10,\"dosing_interval\":2,\"day_and_night\":false,\"day_target_value\":null,\"night_target_value\":null,\"target_value\":5,\"pumps\":{\"pump_1_enabled\":true,\"pump_2_enabled\":false}},\"alarm_min\":3,\"alarm_max\":7}},{\"ec\":{\"monitoring_only\":false,\"control\":{\"dosing_time\":3,\"dosing_interval\":50,\"day_and_night\":true,\"day_target_value\":23,\"night_target_value\":4,\"target_value\":null,\"pumps\":{\"pump1\":{\"enabled\":true,\"value\":10},\"pump2\":{\"enabled\":false,\"value\":null},\"pump3\":{\"enabled\":false,\"value\":null},\"pump4\":{\"enabled\":false,\"value\":null},\"pump5\":{\"enabled\":false,\"value\":null}}},\"alarm_min\":1.5,\"alarm_max\":4}}]}";
	cJSON *root = cJSON_Parse(data_string);
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

void data_handler(char *topic, uint32_t topic_len, char *data, uint32_t data_len) {
	const char *TAG = "DATA_HANDLER";

	char topic_temp[topic_len-1];
	char data_temp[data_len];

	strncpy(topic_temp, topic, topic_len-1);
	strncpy(data_temp, data, data_len);

	if(/*strcmp(topic_temp, settings_data_topic)*/0 == 0) {
		update_settings();
	} else {
		ESP_LOGE(TAG, "Topic not recognized");
	}
}

void create_sensor_data_topic() {
	const char *TAG = "SENSOR_DATA_TOPIC_CREATOR";

	// Create and structure topic for publishing data through MQTT
	char *topic = NULL;
	create_str(&topic, cluster_id);
	append_str(&topic, "/");
	append_str(&topic, "devices/");
	append_str(&topic, device_id);

	// Assign variable
	strcpy(sensor_data_topic, topic);
	ESP_LOGI(TAG, "Sensor data topic: %s", sensor_data_topic);

	// Free memory
	free(topic);

}

void create_settings_data_topic() {
	const char *TAG = "SETTINGS_DATA_TOPIC_CREATOR";

	// Create and structure topic for publishing data through MQTT
	char *topic = NULL;
	create_str(&topic, cluster_id);
	append_str(&topic, "/");
	append_str(&topic, device_id);
	append_str(&topic, "/");
	append_str(&topic, "device_settings");

	// Assign variable
	strcpy(settings_data_topic, "test");
	ESP_LOGI(TAG, "Settings data topic: %s", settings_data_topic);

	// Free memory
	free(topic);

}
