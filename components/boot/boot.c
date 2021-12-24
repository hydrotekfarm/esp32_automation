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

#include "task_priorities.h"
#include "ports.h"
#include "ec_reading.h"
#include "ph_reading.h"
#include "water_temp_reading.h"
#include "sync_sensors.h"
#include "reservoir_control.h"
#include "ec_control.h"
#include "ph_control.h"
#include "control_task.h"
#include "rtc.h"
#include "rf_transmitter.h"
#include "mqtt_manager.h"
#include "network_settings.h"
#include "nvs_manager.h"
#include "deep_sleep_manager.c"
#include "ota.h"

void boot_sequence() {
	// Init OTA
	init_ota();

	// Init nvs
	init_nvs();

	// Initialize deep sleep
	init_power_button();

	const esp_partition_t *running = esp_ota_get_running_partition();
	esp_app_desc_t running_app_info;
	if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
		ESP_LOGI("BOOT ASDF", "Running firmware version: %s", running_app_info.version);
	} else {
		ESP_LOGI("BOOT ASDF", "Couldn't get running firmware");
	}

	// Init connections
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	// Init network properties
	init_network_connections();

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
	xTaskCreatePinnedToCore(sensor_control, "sensor_control_task", 3000, NULL, SENSOR_CONTROL_TASK_PRIORITY, &sensor_control_task_handle, 0);

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
