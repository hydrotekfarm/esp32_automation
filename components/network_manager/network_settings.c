#include "network_settings.h"

#include <esp_err.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>

#include "nvs_manager.h"
#include "nvs_namespace_keys.h"
#include "access_point.h"
#include "wifi_connect.h"
#include "mqtt_manager.h"

struct Network_Settings* get_network_settings() { return &network_settings; }

void init_network_connections() {
	const char *TAG = "INIT_NETWORK_PROPERTIES";

	// Check if initial properties haven't been initialized before
	uint8_t init_properties_status;
	if(!nvs_get_data(&init_properties_status, SYSTEM_SETTINGS_NVS_NAMESPACE, INIT_PROPERTIES_KEY, UINT8) || init_properties_status == 0) {
		ESP_LOGI(TAG, "Properties not initialized. Starting access point");

		do {
			// Creates access point for mobile connection to receive wifi SSID and pw, broker IP address, and station name
			init_access_point_mode();
		} while(!connect_wifi());

		push_network_settings();

	} else {
		pull_network_settings();
		connect_wifi();
	}

	// Initialize and connect to MQTT
	init_mqtt();
	mqtt_connect();

	ESP_LOGI(TAG, "Init properties done");
}

void push_network_settings() {
	uint8_t network_settings_status = 1;

	struct NVS_Data *data = nvs_init_data();

	nvs_add_data(data, WIFI_SSID_KEY, STRING, network_settings.wifi_ssid);
	nvs_add_data(data, WIFI_PW_KEY, STRING, network_settings.wifi_pw);
	nvs_add_data(data, DEVICE_ID_KEY, STRING, network_settings.device_id);
	nvs_add_data(data, BROKER_IP_KEY, STRING, network_settings.broker_ip);
	nvs_add_data(data, INIT_PROPERTIES_KEY, UINT8, &network_settings_status);

	nvs_commit_data(data, SYSTEM_SETTINGS_NVS_NAMESPACE);
}

void pull_network_settings() {
	nvs_get_data(network_settings.wifi_ssid, SYSTEM_SETTINGS_NVS_NAMESPACE, WIFI_SSID_KEY, STRING);
	nvs_get_data(network_settings.wifi_pw, SYSTEM_SETTINGS_NVS_NAMESPACE, WIFI_PW_KEY, STRING);
	nvs_get_data(network_settings.device_id, SYSTEM_SETTINGS_NVS_NAMESPACE, DEVICE_ID_KEY, STRING);
	nvs_get_data(network_settings.broker_ip, SYSTEM_SETTINGS_NVS_NAMESPACE, BROKER_IP_KEY, STRING);
}

void set_invalid_network_settings() {
	uint8_t network_settings_status = 0;

	struct NVS_Data *data = nvs_init_data();
	nvs_add_data(data, INIT_PROPERTIES_KEY, UINT8, &network_settings_status);
	nvs_commit_data(data, SYSTEM_SETTINGS_NVS_NAMESPACE);
}
