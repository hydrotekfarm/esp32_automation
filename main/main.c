#include <inttypes.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <mqtt_client.h>
#include <lwip/sockets.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>
#include <esp_adc_cal.h>
#include <stdbool.h>

#include "ec_sensor.h"
#include "ds18x20.h"
#include "ph_sensor.h"
#include "ultrasonic.h"

static const char *_TAG = "Main";

// WiFi Coordination with Event Group
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT (1<<0)
#define WIFI_FAIL_BIT      (1<<1)

// Sensor Task Coordination with Event Group
static EventGroupHandle_t sensor_event_group;
#define TEMPERATURE_COMPLETED_BIT	(1<<0)
#define EC_COMPLETED_BIT 	        (1<<1)
#define DELAY_COMPLETED_BIT		    (1<<2)
#define PH_COMPLETED_BIT		    (1<<3)
#define ALL_SYNC_BITS ( TEMPERATURE_COMPLETED_BIT | EC_COMPLETED_BIT | DELAY_COMPLETED_BIT | PH_COMPLETED_BIT)

// Core 1 Task Priorities
#define ULTRASONIC_TASK_PRIORITY 1
#define PH_TASK_PRIORITY 2
#define EC_TASK_PRIORITY 3
#define TEMPERATURE_TASK_PRIORITY 4
#define SYNC_TASK_PRIORITY 5

#define MAX_DISTANCE_CM 500

// GPIO and ADC Ports
#define ULTRASONIC_TRIGGER_GPIO 18		// GPIO 18
#define ULTRASONIC_ECHO_GPIO 17			// GPIO 17
#define TEMPERATURE_SENSOR_GPIO 19		// GPIO 19
#define EC_SENSOR_GPIO ADC_CHANNEL_0    // GPIO 36
#define PH_SENSOR_GPIO ADC_CHANNEL_3    // GPIO 39

#define RETRYMAX 5 // WiFi MAX Reconnection Attempts
#define DEFAULT_VREF 1100  // ADC Voltage Reference

static int retryNumber = 0;  // WiFi Reconnection Attempts

static esp_adc_cal_characteristics_t *adc_chars;  // ADC 1 Configuration Settings

// Sensor Measurement Variables
static float _temp = 0;
static float _ec = 0;
static float _distance = 0;
static float _ph = 0;

// Task Handles
static TaskHandle_t temperature_task_handle = NULL;
static TaskHandle_t ec_task_handle = NULL;
static TaskHandle_t ph_task_handle = NULL;
static TaskHandle_t ultrasonic_task_handle = NULL;
static TaskHandle_t sync_task_handle = NULL;
static TaskHandle_t publish_task_handle = NULL;

// Sensor Active Status
static bool temperature_active = true;
static bool ec_active = true;
static bool ph_active = true;

// Sensor Calibration Status
static bool ec_calibration = false;
static bool ph_calibration = false;

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

static void mqtt_event_handler(void *arg, esp_event_base_t event_base,		// MQTT Event Callback Functions
		int32_t event_id, void *event_data) {
	const char *TAG = "MQTT_Event_Handler";
	switch (event_id) {
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
		//if event data = ec_calibration = true
		if (true) {
			ec_calibration = true;
		}
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

void add_sensor_data(esp_mqtt_client_handle_t client, char topic[], float value) {	// Publish Sensor Data Method
	// Convert sensor data to string
	char data[8];
	snprintf(data, sizeof(data), "%.2f", value);

	// Publish string using mqtt client
	esp_mqtt_client_publish(client, topic, data, 0, 1, 0);
}

void publish_data(void *parameter) {			// MQTT Setup and Data Publishing Task
	const char *TAG = "Publisher";

	// Set broker configuration
	esp_mqtt_client_config_t mqtt_cfg = { .host = "70.94.9.135", .port = 1883 };

	// Create and initialize mqtt client
	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler,
			client);
	esp_mqtt_client_start(client);
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

	for (;;) {
		// Publish sensor data
		add_sensor_data(client, "temperature", _temp);
		add_sensor_data(client, "distance", _distance);
		ESP_LOGI(TAG, "publishing data");

		// Publish data every 20 seconds
		vTaskDelay(pdMS_TO_TICKS(20000));
	}
}

void measure_temperature(void *parameter) {		// Water Temperature Measurement Task
	const char *TAG = "Temperature_Task";
	ds18x20_addr_t ds18b20_address[1];
	int sensor_count = 0;

	// Scan and setup sensor
	sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO,
			ds18b20_address, 1);
	while (sensor_count < 1) {
		// Keep Scanning until sensor is found
		sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO,
				ds18b20_address, 1);
		vTaskDelay(pdMS_TO_TICKS(1000));
		ESP_LOGE(TAG, "Sensor Not Found");
	}

	for (;;) {
		while (temperature_active) {		// Water Temperature Sensor is Active
			// Perform Temperature Calculation and Read Temperature; vTaskDelay in the source code of this function
			esp_err_t error = ds18x20_measure_and_read(TEMPERATURE_SENSOR_GPIO,
					ds18b20_address[0], &_temp);
			// Error Management
			if (error == ESP_OK) {
				ESP_LOGI(TAG, "temperature: %f\n", _temp);
			} else if (error == ESP_ERR_INVALID_RESPONSE) {
				ESP_LOGE(TAG, "Temperature Sensor Not Connected\n");
			} else if (error == ESP_ERR_INVALID_CRC) {
				ESP_LOGE(TAG, "Invalid CRC, Try Again\n");
			} else {
				ESP_LOGE(TAG, "Unknown Error\n");
			}
			// Sync with other sensor tasks
			//Wait up to 10 seconds to let other tasks end
			xEventGroupSync(sensor_event_group, TEMPERATURE_COMPLETED_BIT,
			ALL_SYNC_BITS, pdMS_TO_TICKS(10000));
		}
		while (!temperature_active) {		// Delay if Water Temperature Sensor is disabled
			vTaskDelay(pdMS_TO_TICKS(5000));
		}
	}
}

void measure_ec(void *parameter) {				// EC Sensor Measurement Task
	const char *TAG = "EC_Task";
	ec_begin();		// Setup EC sensor and get calibrated values stored in NVS

	for (;;) {
		if (ec_active) {		// EC sensor is Active
			uint32_t adc_reading = 0;
			// Get a Raw Voltage Reading
			for (int i = 0; i < 64; i++) {
				adc_reading += adc1_get_raw(EC_SENSOR_GPIO);
			}
			adc_reading /= 64;
			float voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
			_ec = readEC(voltage, _temp);	// Calculate EC from voltage reading
			ESP_LOGI(TAG, "EC: %f\n", _ec);
			// Sync with other sensor tasks
			//Wait up to 10 seconds to let other tasks end
			xEventGroupSync(sensor_event_group, EC_COMPLETED_BIT, ALL_SYNC_BITS,
					pdMS_TO_TICKS(10000));
		} else if (!ec_active && !ec_calibration) {		// Delay if EC sensor is disabled and calibration is not taking place
			vTaskDelay(pdMS_TO_TICKS(5000));
		} else if (ec_calibration) {		// Calibration Mode is activated
			vTaskPrioritySet(ec_task_handle, (configMAX_PRIORITIES - 1));	// Temporarily increase priority so that calibration can take place without interruption
			// Get many Raw Voltage Samples to allow values to stabilize before calibrating
			for (int i = 0; i < 20; i++) {
				uint32_t adc_reading = 0;
				for (int i = 0; i < 64; i++) {
					adc_reading += adc1_get_raw(EC_SENSOR_GPIO);
				}
				adc_reading /= 64;
				float voltage = esp_adc_cal_raw_to_voltage(adc_reading,
						adc_chars);
				_ec = readEC(voltage, _temp);
				ESP_LOGE(TAG, "ec: %f\n", _ec);
				ESP_LOGE(TAG, "\n\n");
				vTaskDelay(pdMS_TO_TICKS(2000));
			}
			esp_err_t error = calibrateEC();	// Calibrate EC sensor using last voltage reading
			// Error Handling Code
			if (error != ESP_OK) {
				ESP_LOGE(TAG, "Calibration Failed: %d", error);
			}
			// Disable calibration mode, activate EC sensor and revert task back to regular priority
			ec_calibration = false;
			ec_active = true;
			vTaskPrioritySet(ec_task_handle, EC_TASK_PRIORITY);
		}
	}
}

void measure_ph(void *parameter) {				// pH Sensor Measurement Task
	const char *TAG = "PH_Task";
	ph_begin();	// Setup pH sensor and get calibrated values stored in NVS

	for (;;) {
		if (ph_active) {	// pH sensor is Active
			uint32_t adc_reading = 0;
			// Get a Raw Voltage Reading
			for (int i = 0; i < 64; i++) {
				adc_reading += adc1_get_raw(PH_SENSOR_GPIO);
			}
			adc_reading /= 64;
			float voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
			ESP_LOGI(TAG, "voltage: %f", voltage);
			_ph = readPH(voltage);		// Calculate pH from voltage Reading
			ESP_LOGI(TAG, "PH: %.4f\n", _ph);
			// Sync with other sensor tasks
			//Wait up to 10 seconds to let other tasks end
			xEventGroupSync(sensor_event_group, PH_COMPLETED_BIT, ALL_SYNC_BITS, pdMS_TO_TICKS(10000));
		} else if (!ph_active && !ph_calibration) {	// Delay if pH sensor is disabled and calibration is not taking place
			vTaskDelay(pdMS_TO_TICKS(5000));
		} else if (ph_calibration) {	// Calibration Mode is activated
			ESP_LOGI(TAG, "Start Calibration");
			vTaskPrioritySet(ph_task_handle, (configMAX_PRIORITIES - 1));	// Temporarily increase priority so that calibration can take place without interruption
			// Get many Raw Voltage Samples to allow values to stabilize before calibrating
			for (int i = 0; i < 20; i++) {
				uint32_t adc_reading = 0;
				for (int i = 0; i < 64; i++) {
					adc_reading += adc1_get_raw(PH_SENSOR_GPIO);
				}
				adc_reading /= 64;
				float voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
				ESP_LOGI(TAG, "voltage: %f", voltage);
				_ph = readPH(voltage);
				ESP_LOGI(TAG, "pH: %.4f\n", _ph);
				vTaskDelay(pdMS_TO_TICKS(1000));
			}
			esp_err_t error = calibratePH();	// Calibrate pH sensor using last voltage reading
			// Error Handling Code
			if (error != ESP_OK) {
				ESP_LOGE(TAG, "Calibration Failed: %d", error);
			}
			// Disable calibration mode, activate pH sensor and revert task back to regular priority
			ph_calibration = false;
			ph_active = true;
			vTaskPrioritySet(ph_task_handle, PH_TASK_PRIORITY);
		}
	}
}

void sync_task(void *parameter) {				// Sensor Synchronization Task
	const char *TAG = "Sync_Task";
	EventBits_t returnBits;
	for (;;) {
		// Provide a minimum amount of time before event group round resets
		vTaskDelay(pdMS_TO_TICKS(10000));
		returnBits = xEventGroupSync(sensor_event_group, DELAY_COMPLETED_BIT, ALL_SYNC_BITS, pdMS_TO_TICKS(10000));
		// Check Whether all tasks were completed on time
		if ((returnBits & ALL_SYNC_BITS) == ALL_SYNC_BITS) {
			ESP_LOGI(TAG, "Completed");
		} else {
			ESP_LOGE(TAG, "Failed to Complete On Time");
		}
	}
}

void measure_distance(void *parameter) {		// Ultrasonic Sensor Distance Measurement Task
	const char *TAG = "ULTRASONIC_TAG";

	// Setting sensor ports and initializing
	ultrasonic_sensor_t sensor = { .trigger_pin = ULTRASONIC_TRIGGER_GPIO,
			.echo_pin = ULTRASONIC_ECHO_GPIO };

	ultrasonic_init(&sensor);
	ESP_LOGI(TAG, "Ultrasonic initialized\n");

	for (;;) {
		// Measures distance and returns error code
		float distance;
		esp_err_t res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);

		// TODO check if value is beyond acceptable margin of error and react appropriately

		// React appropriately to error code
		switch (res) {
		case ESP_ERR_ULTRASONIC_PING:
			ESP_LOGE(TAG, "Device is in invalid state");
			break;
		case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
			ESP_LOGE(TAG, "Device not found");
			break;
		case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
			ESP_LOGE(TAG, "Distance is too large");
			break;
		default:
			ESP_LOGE(TAG, "Distance: %f cm\n", distance);
			_distance = distance;
		}

		// Measure in 10 sec increments
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}

void port_setup() {								// ADC Port Setup Method
	// ADC 1 setup
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12,
			DEFAULT_VREF, adc_chars);

	// ADC Channel Setup
	adc1_config_channel_atten(ADC_CHANNEL_0, ADC_ATTEN_DB_11);
	adc1_config_channel_atten(ADC_CHANNEL_3, ADC_ATTEN_DB_11);
}

void app_main(void) {							// Main Method
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
				.ssid = "MySpectrumWiFic0-2G",
				.password = "bluebrain782" },
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
				ESP_LOGE(_TAG,
						"EFUSE_VREF not supported, use a different ESP 32 board");
			}

			// Create core 0 tasks
			xTaskCreatePinnedToCore(publish_data, "publish_task", 2500, NULL, 1, &publish_task_handle, 0);

			// Create core 1 tasks
			xTaskCreatePinnedToCore(measure_temperature, "temperature_task", 2500, NULL, TEMPERATURE_TASK_PRIORITY, &temperature_task_handle, 1);
			xTaskCreatePinnedToCore(measure_ec, "ec_task", 2500, NULL, EC_TASK_PRIORITY, &ec_task_handle, 1);
			xTaskCreatePinnedToCore(measure_ph, "ph_task", 2500, NULL, PH_TASK_PRIORITY, &ph_task_handle, 1);
			xTaskCreatePinnedToCore(sync_task, "sync_task", 2500, NULL, SYNC_TASK_PRIORITY, &sync_task_handle, 1);
			xTaskCreatePinnedToCore(measure_distance, "ultrasonic_task", 2500, NULL, ULTRASONIC_TASK_PRIORITY, &ultrasonic_task_handle, 1);

		} else if ((eventBits & WIFI_FAIL_BIT) != 0) {
			ESP_LOGE("", "WIFI Connection Failed\n");
		} else {
			ESP_LOGE("", "Unexpected Event\n");
		}
	}
