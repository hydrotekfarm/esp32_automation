#include "boot.h"

#include <esp_err.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <freertos/event_groups.h>
#include <string.h>
#include <driver/gpio.h>
#include <stdio.h>

#include "task_priorities.h"
#include "ports.h"
#include "ec_reading.h"
#include "ph_reading.h"
#include "ultrasonic_reading.h"
#include "water_temp_reading.h"
#include "sync_sensors.h"
#include "ec_control.h"
#include "ph_control.h"
#include "mqtt_manager.h"
#include "control_task.h"
#include "rtc.h"
#include "rf_transmitter.h"
#include "mcp23x17.h"

static void event_handler(void *arg, esp_event_base_t event_base,		// WiFi Event Handler
		int32_t event_id, void *event_data) {
	const char *TAG = "Event_Handler";
	ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%d\n",
			event_base, event_id);

	// Check Event Type
	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got IP:%s", ip4addr_ntoa(&event->ip_info.ip));
		retryNumber = 0;
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		ESP_ERROR_CHECK(esp_wifi_connect());
		retryNumber = 0;
	} else if (event_base == WIFI_EVENT
			&& event_id == WIFI_EVENT_STA_DISCONNECTED) {
		// Attempt Reconnection
		if (retryNumber < RETRYMAX) {
			esp_wifi_connect();
			retryNumber++;
		} else {
			xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG, "WIFI Connection Failed; Reconnecting....\n");
	}
}

void boot_sequence() {
	const char *TAG = "BOOT_SEQUENCE";

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
	retryNumber = 0;
	esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
	esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	wifi_config_t wifi_config = { .sta = {
			.ssid = "superhero",
			.password = "GeminiCircus" },
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

		// Init i2cdev
		ESP_ERROR_CHECK(i2cdev_init());

		// ADC 1 setup
		adc1_config_width(ADC_WIDTH_BIT_12);
		adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
		esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12,
				DEFAULT_VREF, adc_chars);

		// ADC Channel Setup
		adc1_config_channel_atten(EC_SENSOR_GPIO, ADC_ATTEN_DB_11);
		adc1_config_channel_atten(PH_SENSOR_GPIO, ADC_ATTEN_DB_11);

		esp_err_t error = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF); 	// Check if built in ADC calibration is included in board
		if (error != ESP_OK) {
			ESP_LOGE(TAG, "EFUSE_VREF not supported, use a different ESP 32 board");
		}

		// Initialize MCP23017 GPIO Expansion
//		mcp23x17_t dev;
//		memset(&dev, 0, sizeof(mcp23x17_t));
//		ESP_ERROR_CHECK(mcp23x17_init_desc(&dev, 0, MCP23X17_ADDR_BASE, SDA_GPIO, SCL_GPIO));
//
//		// Initialize GPIO Expansion Ports
//		mcp23x17_set_mode(&dev, PH_UP_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
//		mcp23x17_set_mode(&dev, PH_DOWN_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
//		mcp23x17_set_mode(&dev, EC_NUTRIENT_1_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
//		mcp23x17_set_mode(&dev, EC_NUTRIENT_2_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
//		mcp23x17_set_mode(&dev, EC_NUTRIENT_3_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
//		mcp23x17_set_mode(&dev, EC_NUTRIENT_4_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
//		mcp23x17_set_mode(&dev, EC_NUTRIENT_5_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);
//		mcp23x17_set_mode(&dev, EC_NUTRIENT_6_PUMP_GPIO, MCP23X17_GPIO_OUTPUT);

		is_day = true;

		ultrasonic_active = true;

		ph_control_active = false;
		ph_day_night_control = false;
		ec_control_active = false;
		ec_day_night_control = true;
		night_target_ec = 3;

		// Set all sync bits var
		set_sensor_sync_bits();

		// Init rtc and check if time needs to be set
		init_rtc();
		check_rtc_reset();

		// Init rf transmitter
		init_rf();

//		// Create core 0 tasks
		xTaskCreatePinnedToCore(manage_timers_alarms, "timer_alarm_task", 2500, NULL, TIMER_ALARM_TASK_PRIORITY, &timer_alarm_task_handle, 0);
		xTaskCreatePinnedToCore(publish_data, "publish_task", 2500, NULL, MQTT_PUBLISH_TASK_PRIORITY, &publish_task_handle, 0);
		xTaskCreatePinnedToCore(sensor_control, "sensor_control_task", 2500, NULL, SENSOR_CONTROL_TASK_PRIORITY, &sensor_control_task_handle, 0);

		// Create core 1 tasks
		//xTaskCreatePinnedToCore(measure_water_temperature, "temperature_task", 2500, NULL, WATER_TEMPERATURE_TASK_PRIORITY, &water_temperature_task_handle, 1);
		xTaskCreatePinnedToCore(measure_ec, "ec_task", 2500, NULL, EC_TASK_PRIORITY, sensor_get_task_handle(get_ec_sensor()), 1);
		xTaskCreatePinnedToCore(measure_ph, "ph_task", 2500, NULL, PH_TASK_PRIORITY, &ph_task_handle, 1);
		// if(ultrasonic_active) xTaskCreatePinnedToCore(measure_distance, "ultrasonic_task", 2500, NULL, ULTRASONIC_TASK_PRIORITY, &ultrasonic_task_handle, 1);
		xTaskCreatePinnedToCore(sync_task, "sync_task", 2500, NULL, SYNC_TASK_PRIORITY, &sync_task_handle, 1);

	} else if ((eventBits & WIFI_FAIL_BIT) != 0) {
		ESP_LOGE(TAG, "WIFI Connection Failed\n");
	} else {
		ESP_LOGE(TAG, "Unexpected Event\n");
	}
}

void restart_esp32() { // Restart ESP32
	ESP_LOGE("", "RESTARTING ESP32");
	fflush(stdout);
	esp_restart();
}
