#include "mqtt_manager.h"

#include <mqtt_client.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/semphr.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cjson.h>

#include "boot.h"
#include "ec_reading.h"
#include "ph_reading.h"
#include "ultrasonic_reading.h"
#include "water_temp_reading.h"
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

	cluster_id = "Grow Room 1";
	device_id = "System 1";

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

	// Subscribe to topics
	esp_mqtt_client_subscribe(client, "settings_data", SUBSCRIBE_DATA_QOS);

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

		// Free allocated memory
		free(data);
		data = NULL;

		// Publish data every sensor reading
		vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}

void data_handler(char *topic, uint32_t topic_len, char *data, uint32_t data_len) {
	const char *TAG = "DATA_HANDLER";

	topic[topic_len] = '\0';
	data[data_len] = '\0';

	if(topic == settings_data_topic) {
		ESP_LOGI(TAG, "Settings data received: %s", data);
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
	append_str(&topic, "systems/");
	append_str(&topic, device_id);

	// Assign variable
	sensor_data_topic = topic;
	ESP_LOGI(TAG, "Topic: %s", sensor_data_topic);

	// Free memory
	free(topic);
}

void create_settings_data_topic() {
	const char *TAG = "SETTINGS_DATA_TOPIC_CREATOR";

	settings_data_topic = "settings_data";
	ESP_LOGI(TAG, "Topic is: %s", settings_data_topic);
}
