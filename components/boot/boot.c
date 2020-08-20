#include "boot.h"

#include <esp_err.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_wifi.h>
#include <esp_adc_cal.h>
#include <nvs_flash.h>

//#include "main.c"

void boot_sequence() {
	char *TAG = "BOOT";

	// Check if space available in NVS, if not reset NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES
			|| ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Initialize TCP IP stack and create WiFi management event loop
	tcpip_adapter_init();
	esp_event_loop_create_default();
	wifi_event_group = xEventGroupCreate();

	// Initialize WiFi and configure WiFi connection settings
	const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	// TODO: Update to esp_event_handler_instance_register()
	esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
	esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	wifi_config_t wifi_config = { .sta = {
			.ssid = "LeawoodGuest",
			.password = "guest,123" },
	};
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	// Do not proceed until WiFi is connected
	EventBits_t eventBits;
	eventBits = xEventGroupWaitBits(wifi_event_group,
	WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE,
	portMAX_DELAY);

	if ((eventBits & WIFI_CONNECTED_BIT) != 0) {
		sensor_event_group = xEventGroupCreate();

		port_setup();	// Setup ADC ports
		esp_err_t error = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF); 	// Check if built in ADC calibration is included in board
		if (error != ESP_OK) {
			ESP_LOGE(TAG,
					"EFUSE_VREF not supported, use a different ESP 32 board");
		}

		// Setup gpio ports
		gpio_setup();

		// Set all sync bits var
		set_sensor_sync_bits(&sensor_sync_bits);

		// Init i2cdev
		ESP_ERROR_CHECK(i2cdev_init());

		// Init rtc and check if time needs to be set
		init_rtc();
		check_rtc_reset();

//		// Create core 0 tasks
//		xTaskCreatePinnedToCore(manage_timers_alarms, "timer_alarm_task", 2500, NULL, TIMER_ALARM_TASK_PRIORITY, &timer_alarm_task_handle, 0);
//		xTaskCreatePinnedToCore(publish_data, "publish_task", 2500, NULL, MQTT_PUBLISH_TASK_PRIORITY, &publish_task_handle, 0);
//		xTaskCreatePinnedToCore(sensor_control, "sensor_control_task", 2500, NULL, SENSOR_CONTROL_TASK_PRIORITY, &sensor_control_task_handle, 0);
//
//		// Create core 1 tasks
//		if(water_temperature_active) xTaskCreatePinnedToCore(measure_water_temperature, "temperature_task", 2500, NULL, WATER_TEMPERATURE_TASK_PRIORITY, &water_temperature_task_handle, 1);
//		if(ec_active) xTaskCreatePinnedToCore(measure_ec, "ec_task", 2500, NULL, EC_TASK_PRIORITY, &ec_task_handle, 1);
//		if(ph_active) xTaskCreatePinnedToCore(measure_ph, "ph_task", 2500, NULL, PH_TASK_PRIORITY, &ph_task_handle, 1);
//		if(ultrasonic_active) xTaskCreatePinnedToCore(measure_distance, "ultrasonic_task", 2500, NULL, ULTRASONIC_TASK_PRIORITY, &ultrasonic_task_handle, 1);
//		xTaskCreatePinnedToCore(sync_task, "sync_task", 2500, NULL, SYNC_TASK_PRIORITY, &sync_task_handle, 1);

	} else if ((eventBits & WIFI_FAIL_BIT) != 0) {
		ESP_LOGE("", "WIFI Connection Failed\n");
	} else {
		ESP_LOGE("", "Unexpected Event\n");
	}
}



