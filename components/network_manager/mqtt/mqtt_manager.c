#include "mqtt_manager.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/semphr.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

void mqtt_event_handler(esp_mqtt_event_handle_t event) {
	const char *TAG = "MQTT_Event_Handler";
	switch (event->event_id) {
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "Connected\n");
		xSemaphoreGive(mqtt_connect_semaphore);
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

void make_topics() {
	ESP_LOGI("", "Starting make topics");

	int device_id_len = strlen(get_network_settings()->device_id);

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
	ESP_LOGI(MQTT_TAG, "Sensor settings topic: %s", grow_cycle_topic);

	init_topic(&rf_control_topic, device_id_len + 1 + strlen(RF_CONTROL_HEADING) + 1, RF_CONTROL_HEADING);
	add_id(rf_control_topic);
	ESP_LOGI(MQTT_TAG, "RF control settings topic: %s", rf_control_topic);
}

void subscribe_topics() {
	// Subscribe to topics
	esp_mqtt_client_subscribe(mqtt_client, sensor_settings_topic, SUBSCRIBE_DATA_QOS);
	esp_mqtt_client_subscribe(mqtt_client, grow_cycle_topic, SUBSCRIBE_DATA_QOS);
	esp_mqtt_client_subscribe(mqtt_client, rf_control_topic, SUBSCRIBE_DATA_QOS);
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
	xSemaphoreTake(mqtt_connect_semaphore, portMAX_DELAY); // TODO add approximate time to connect to mqtt

	// Subscribe to topics
	subscribe_topics();

	// Send connect success message (must be retain message)
	esp_mqtt_client_publish(mqtt_client, wifi_connect_topic, "1", 0, PUBLISH_DATA_QOS, 1);

	// Send equipment statuses
	publish_equipment_status();

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
	cJSON *arr = root->child;

	for(int i = 0; i < cJSON_GetArraySize(arr); i++) {
		cJSON *subitem = cJSON_GetArrayItem(arr, i)->child;
		char *data_topic = subitem->string;

		if(strcmp("ph", data_topic) == 0) {
			ESP_LOGI(MQTT_TAG, "pH data received");
			ph_update_settings(subitem);
		} else if(strcmp("ec", data_topic) == 0) {
			ESP_LOGI(MQTT_TAG, "ec data received");
			ec_update_settings(subitem);
		} else if(strcmp("water_temp", data_topic) == 0) {
			ESP_LOGI(MQTT_TAG, "water temp data received");
			water_temp_update_settings(subitem);
		} else if(strcmp("irrigation", data_topic) == 0) {
			update_irrigation_timings(subitem);
		} else if(strcmp("grow_lights", data_topic) == 0) {
			update_grow_light_timings(subitem);
		} else {
			ESP_LOGE(MQTT_TAG, "Data %s not recognized", data_topic);
		}
	}
	cJSON_Delete(root);

	ESP_LOGI(MQTT_TAG, "Settings updated");
	if(!get_is_settings_received()) settings_received();
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
    } else if(strcmp(topic, grow_cycle_topic) == 0) {
        ESP_LOGI(TAG, "Grow cycle status received");

        // Start/stop grow cycle according to message
        if(data[0] == '0') stop_grow_cycle();
        else start_grow_cycle();
    } else if(strcmp(topic, rf_control_topic) == 0) {
        cJSON *obj = cJSON_Parse(data);
        obj = obj->child;
        ESP_LOGI(TAG, "RF id number %d: RF state: %d", atoi(obj->string), obj->valueint);
        control_power_outlet(atoi(obj->string), obj->valueint);
    } else if(strcmp(topic, calibration_topic) == 0) {
        cJSON *obj = cJSON_Parse(data);
        update_calibration(obj); 
    } else {
        // Topic doesn't match any known topics
        ESP_LOGE(TAG, "Topic unknown");
    }

    free(topic);
    free(data);
}

void update_calibration(cJSON *data) {
    cJSON *obj = data->child; 
    char *data_string = cJSON_Print(data);
    ESP_LOGI(MQTT_TAG, "%s", data_string);
    if (strcmp(obj->string, "type") == 0) {
        if (strcmp(obj->valuestring, "ph") == 0) {
            sensor_set_calib_status(&ph_sensor, true);
            ESP_LOGI(MQTT_TAG, "pH calibration received");
            if (!is_grow_active) {
                vTaskResume(*sensor_get_task_handle(&ph_sensor));
                ESP_LOGI(MQTT_TAG, "pH task resumed");
            }
        } else if (strcmp(obj->valuestring, "ec_wet") == 0) {
            sensor_set_calib_status(&ec_sensor, true);
            ESP_LOGI(MQTT_TAG, "ec wet calibration received");
            if (!is_grow_active) {
                vTaskResume(*sensor_get_task_handle(&ec_sensor));
                ESP_LOGI(MQTT_TAG, "ec task resumed");
            }
        } else if (strcmp(obj->valuestring, "ec_dry") == 0) {
            dry_calib = true; 
            ESP_LOGI(MQTT_TAG, "ec dry calibration received");
            if (!is_grow_active) {
                vTaskResume(*sensor_get_task_handle(&ec_sensor));
                ESP_LOGI(MQTT_TAG, "ec task resumed");
            }
        } else {
            ESP_LOGE(MQTT_TAG, "Invalid Value Recieved");
        }
    } else {
        ESP_LOGE(MQTT_TAG, "Invalid Key Recieved, Expected Key: type");
    }
    cJSON_Delete(data);
}


