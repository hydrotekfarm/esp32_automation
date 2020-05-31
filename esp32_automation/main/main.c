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

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT (1<<0)
#define WIFI_FAIL_BIT      (1<<1)

static EventGroupHandle_t sensor_event_group;
#define TEMPERATURE_COMPLETED_BIT	(1<<0)
#define EC_COMPLETED_BIT 	        (1<<1)
#define DELAY_COMPLETED_BIT		    (1<<2)
#define ALL_SYNC_BITS ( TEMPERATURE_COMPLETED_BIT | EC_COMPLETED_BIT | DELAY_COMPLETED_BIT )

#define EC_TASK_PRIORITY 2
#define TEMPERATURE_TASK_PRIORITY 3

#define MAX_DISTANCE_CM 500
#define ULTRASONIC_TRIGGER_GPIO 18
#define ULTRASONIC_ECHO_GPIO 17

#define RETRYMAX 5
#define DEFAULT_VREF 1100
static int retryNumber = 0;

static esp_adc_cal_characteristics_t *adc_chars;

static float _temp = 0;
static float ec = 0;


static TaskHandle_t temperature_task_handle = NULL;
static TaskHandle_t publish_task_handle = NULL;
static TaskHandle_t ec_task_handle = NULL;
static TaskHandle_t ph_task_handle = NULL;
static TaskHandle_t ultrasonic_task_handle = NULL;

static bool temperature_active = true;
static bool ec_active = true;

static bool ec_calibration = false;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	const char * TAG = "Event_Handler";
	ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%d\n", event_base, event_id);
	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got IP:%s", ip4addr_ntoa(&event->ip_info.ip));
		retryNumber = 0;
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
	}
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		ESP_ERROR_CHECK(esp_wifi_connect());
		retryNumber = 0;

	}
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
		if(retryNumber < RETRYMAX){
			esp_wifi_connect();
			retryNumber++;
		} else {
			xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG, "WIFI Connection Failed; Reconnecting....\n");
	}
}

static void mqtt_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	const char * TAG = "MQTT_Event_Handler";
	switch(event_id){
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
		if(true){
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

void publish_data(void * parameter){
	esp_mqtt_client_config_t mqtt_cfg = {
			.host = "192.168.1.16",
			.port = 1883
	};
	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
	esp_mqtt_client_start(client);
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
	for(;;){
		char temperature[8];
		snprintf(temperature, sizeof(temperature), "%.4f", _temp);
		esp_mqtt_client_publish(client, "sen", temperature, 0, 1, 0);
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}

void measure_temperature(void * parameter){
	const char * TAG = "Temperature_Task";
	ds18x20_addr_t ds18b20_address[1];
	int sensor_count = 0;

	sensor_count = ds18x20_scan_devices(18, ds18b20_address, 1);
	while(sensor_count < 1){
		sensor_count = ds18x20_scan_devices(18, ds18b20_address, 1);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	for(;;){
		while(temperature_active){
			esp_err_t error = ds18x20_measure_and_read(18, ds18b20_address[0] , &_temp);
			if(error == ESP_OK){
				ESP_LOGI(TAG, "temperature: %f\n", _temp);
			} else if(error == ESP_ERR_INVALID_RESPONSE){
				ESP_LOGE(TAG, "Temperature Sensor not Connected\n");
			} else if(error == ESP_ERR_INVALID_CRC){
				ESP_LOGE(TAG, "Invalid CRC, Try Again\n");
			} else {
				ESP_LOGE(TAG, "Unknown Error\n");
			}
			xEventGroupSync(sensor_event_group, TEMPERATURE_COMPLETED_BIT, ALL_SYNC_BITS, pdMS_TO_TICKS(10000));
		}
		while(!temperature_active){
			vTaskDelay(pdMS_TO_TICKS(5000));
		}
	}
}


void measure_ec(void * parameter){
	const char * TAG = "EC_Task";
	ec_begin();

	for(;;){
		if(ec_active){
			uint32_t adc_reading = 0;
			for (int i = 0; i < 64; i++) {
				adc_reading += adc1_get_raw(ADC_CHANNEL_0);
			}
			adc_reading /= 64;
			float voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
			ec = readEC(voltage, _temp);
			ESP_LOGI(TAG, "EC: %f\n", ec);
			xEventGroupSync(sensor_event_group, EC_COMPLETED_BIT, ALL_SYNC_BITS, pdMS_TO_TICKS(10000));
		} else if(!ec_active && !ec_calibration){
			vTaskDelay(pdMS_TO_TICKS(5000));
		} else if(ec_calibration){
			vTaskPrioritySet(ec_task_handle, (configMAX_PRIORITIES - 1));
			for(int i = 0; i < 20; i++){
				uint32_t adc_reading = 0;
				for (int i = 0; i < 64; i++) {
					adc_reading += adc1_get_raw(ADC_CHANNEL_0);
				}
				adc_reading /= 64;
				float voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
				ec = readEC(voltage, _temp);
				ESP_LOGE(TAG, "ec: %f\n", ec);
				ESP_LOGE(TAG, "\n\n");
				vTaskDelay(pdMS_TO_TICKS(2000));
			}
			esp_err_t error = calibrateEC();
			if(error != ESP_OK){
				ESP_LOGE(TAG, "Calibration Failed: %d", error);
			}
			ec_calibration = false;
			vTaskPrioritySet(ec_task_handle, EC_TASK_PRIORITY);
		}
	}
}

void measure_ph(void * parameter){
	EventBits_t returnBits;
	for(;;){
		vTaskDelay(pdMS_TO_TICKS(2000));
		returnBits = xEventGroupSync(sensor_event_group, DELAY_COMPLETED_BIT, ALL_SYNC_BITS, pdMS_TO_TICKS(10000));
		if((returnBits & ALL_SYNC_BITS) == ALL_SYNC_BITS){
			ESP_LOGI("PH_Task", "Completed");
		}
	}
}

void measure_distance (void * parameter) {
	const char * TAG = "ULTRASONIC_TAG";
	ultrasonic_sensor_t sensor = {
			.trigger_pin = ULTRASONIC_TRIGGER_GPIO,
			.echo_pin = ULTRASONIC_ECHO_GPIO
	};

	ultrasonic_init(&sensor);
	ESP_LOGE(TAG, "Ultrasonic initialized\n");

	for(;;) {
		uint32_t distance;
		esp_err_t res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
		ESP_LOGE(TAG, "Distance: %d cm\n", distance);

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

void port_setup(){
	// ADC 1 setup
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC_CHANNEL_1, ADC_ATTEN_DB_11);
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}

void app_main(void)
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	tcpip_adapter_init();
	esp_event_loop_create_default();


	wifi_event_group = xEventGroupCreate();

	const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	// TODO: Update to esp_event_handler_instance_register()
	esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
	esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	wifi_config_t wifi_config = {
			.sta = {
					.ssid = "LeawoodGuest",
					.password = "guest,123"
			},
	};
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	EventBits_t eventBits;
	eventBits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
	if((eventBits & WIFI_CONNECTED_BIT) != 0){
		sensor_event_group = xEventGroupCreate();

		port_setup();
		esp_err_t error = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF);
		if (error != ESP_OK) {
			ESP_LOGE(_TAG, "EFUSE_VREF not supported, use a different ESP 32 board");
		}

		/*xTaskCreatePinnedToCore(measure_temperature, "temperature_task", 2500, NULL, TEMPERATURE_TASK_PRIORITY, &temperature_task_handle, 1);
		xTaskCreatePinnedToCore(measure_ec, "ec_task", 2500, NULL, EC_TASK_PRIORITY, &ec_task_handle, 1);
		xTaskCreatePinnedToCore(measure_ph, "ph_task", 2500, NULL, 4, &ph_task_handle, 1);*/
		xTaskCreatePinnedToCore(measure_distance, "ultrasonic_task", 2500, NULL, 3, &ultrasonic_task_handle, 1);

		xTaskCreatePinnedToCore(publish_data, "publish_task", 2500, NULL, 1, &publish_task_handle, 0);
	}
	else if((eventBits & WIFI_FAIL_BIT) != 0){
		ESP_LOGE("", "WIFI Connection Failed\n");
	} else {
		ESP_LOGE("", "Unexpected Event\n");
	}
}
