#include "boot.h"

#include <esp_err.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <freertos/event_groups.h>
#include <string.h>
#include <driver/gpio.h>
#include <stdio.h>

#include "access_point.h"
#include "task_priorities.h"
#include "ports.h"
#include "ec_reading.h"
#include "ph_reading.h"
#include "ultrasonic_reading.h"
#include "water_temp_reading.h"
#include "sync_sensors.h"
#include "reservoir_control.h"
#include "ec_control.h"
#include "ph_control.h"
#include "control_task.h"
#include "rtc.h"
#include "rf_transmitter.h"
#include "nvs_manager.h"
#include "nvs_namespace_keys.h"
#include "mqtt_manager.h"
#include "wifi_connect.h"

#define ESP_INTR_FLAG_DEFAULT 0

void boot_sequence() {
	const char *TAG = "BOOT_SEQUENCE";

	init_nvs();

	// ---------------------------TEST NVS --------------------------------------

//	ESP_LOGI(TAG, "Testing nvs");
//	struct Data *data = nvs_init_data();
//	uint8_t u8 = 1;
//	int8_t i8 = 2;
//	uint16_t u16 = 3;
//	int16_t i16 = 4;
//	uint32_t u32 = 5;
//	int32_t i32 = 6;
//	uint64_t u64 = 7;
//	int64_t i64 = 8;
//	float fl = 9.6;
//	char str[] = "hello";
//
//	nvs_add_data(data, "u8", UINT8, &u8);
//	nvs_add_data(data, "i8", INT8, &i8);
//	nvs_add_data(data, "u16", UINT16, &u16);
//	nvs_add_data(data, "i16", INT16, &i16);
//	nvs_add_data(data, "u32", UINT32, &u32);
//	nvs_add_data(data, "i32", INT32, &i32);
//	nvs_add_data(data, "u64", UINT64, &u64);
//	nvs_add_data(data, "i64", INT64, &i64);
//	nvs_add_data(data, "float", FLOAT, &fl);
//	nvs_add_data(data, "string", STRING, str);
//
//	nvs_commit_data(data, "test_ns");
//	ESP_LOGI(TAG, "Commited nvs data");
//
//	uint8_t get_u8;
//	nvs_get_data(&get_u8, "test_ns", "u8", UINT8);
//	ESP_LOGI(TAG, "Data received from nvs: %d", get_u8);
//
//	int8_t get_i8;
//	nvs_get_data(&get_i8, "test_ns", "i8", INT8);
//	ESP_LOGI(TAG, "Data received from nvs: %d", get_i8);
//
//	uint16_t get_u16;
//	nvs_get_data(&get_u16, "test_ns", "u16", UINT16);
//	ESP_LOGI(TAG, "Data received from nvs: %d", get_u16);
//
//	int16_t get_i16;
//	nvs_get_data(&get_i16, "test_ns", "i16", INT16);
//	ESP_LOGI(TAG, "Data received from nvs: %d", get_i16);
//
//	uint32_t get_u32;
//	nvs_get_data(&get_u32, "test_ns", "u32", UINT32);
//	ESP_LOGI(TAG, "Data received from nvs: %d", get_u32);
//
//	int32_t get_i32;
//	nvs_get_data(&get_i32, "test_ns", "i32", INT32);
//	ESP_LOGI(TAG, "Data received from nvs: %d", get_i32);
//
//	uint64_t get_u64;
//	nvs_get_data(&get_u64, "test_ns", "u64", UINT64);
//	ESP_LOGI(TAG, "Data received from nvs: %lld", get_u64);
//
//	int64_t get_i64;
//	nvs_get_data(&get_i64, "test_ns", "i64", INT64);
//	ESP_LOGI(TAG, "Data received from nvs: %lld", get_i64);
//
//	float get_fl;
//	nvs_get_data(&get_fl, "test_ns", "float", FLOAT);
//	ESP_LOGI(TAG, "Data received from nvs: %f", get_fl);
//
//	char get_str[10];
//	nvs_get_data(get_str, "test_ns", "string", STRING);
//	ESP_LOGI(TAG, "Data received from nvs: %s", get_str);


	// ---------------------------END TEST --------------------------------------

	// Init connections
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_create_default());


	ESP_LOGI(TAG, "Checking for init properties");
	// Check if initial properties haven't been initialized before
	uint8_t init_properties_status;
	if(!nvs_get_data(&init_properties_status, SYSTEM_SETTINGS_NVS_NAMESPACE, INIT_PROPERTIES_KEY, UINT8) || init_properties_status == 0) {
		ESP_LOGI(TAG, "Properties not initialized. Starting access point");

		// Creates access point for mobile connection to receive wifi SSID and pw, broker IP address, and station name
		init_connect_properties();

		ESP_LOGI(TAG, "Access point done. Updating NVS value");
		init_properties_status = 1;
		struct NVS_Data *data = nvs_init_data();
		nvs_add_data(data, INIT_PROPERTIES_KEY, UINT8, &init_properties_status);

		// TODO add rest of data to be pushed to nvs

		nvs_commit_data(data, SYSTEM_SETTINGS_NVS_NAMESPACE);

		ESP_LOGI(TAG, "NVS value updated");
	}

	ESP_LOGI(TAG, "Init properties done");

	if(!connect_wifi()) return; // TODO handle wifi not connected error

	sensor_event_group = xEventGroupCreate();

	// Init i2cdev
	ESP_ERROR_CHECK(i2cdev_init());

	init_ports();
	is_day = true;

	// Set all sync bits var
	set_sensor_sync_bits();

	// Init rtc and check if time needs to be set
	init_rtc();
	check_rtc_reset();

	// Create core 0 tasks
	xTaskCreatePinnedToCore(rf_transmitter, "rf_transmitter_task", 2500, NULL, RF_TRANSMITTER_TASK_PRIORITY, &rf_transmitter_task_handle, 0);
	xTaskCreatePinnedToCore(manage_timers_alarms, "timer_alarm_task", 2500, NULL, TIMER_ALARM_TASK_PRIORITY, &timer_alarm_task_handle, 0);
	xTaskCreatePinnedToCore(publish_data, "publish_task", 2500, NULL, MQTT_PUBLISH_TASK_PRIORITY, &publish_task_handle, 0);
	xTaskCreatePinnedToCore(sensor_control, "sensor_control_task", 2550, NULL, SENSOR_CONTROL_TASK_PRIORITY, &sensor_control_task_handle, 0);

	// Create core 1 tasks
	xTaskCreatePinnedToCore(measure_water_temperature, "temperature_task", 2500, NULL, WATER_TEMPERATURE_TASK_PRIORITY, sensor_get_task_handle(get_water_temp_sensor()), 1);
	xTaskCreatePinnedToCore(measure_ec, "ec_task", 2500, NULL, EC_TASK_PRIORITY, sensor_get_task_handle(get_ec_sensor()), 1);
	xTaskCreatePinnedToCore(measure_ph, "ph_task", 2500, NULL, PH_TASK_PRIORITY, sensor_get_task_handle(get_ph_sensor()), 1);
	xTaskCreatePinnedToCore(sync_task, "sync_task", 2500, NULL, SYNC_TASK_PRIORITY, &sync_task_handle, 1);
}

void restart_esp32() { // Restart ESP32
	ESP_LOGE("", "RESTARTING ESP32");
	fflush(stdout);
	esp_restart();
}
