#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "../components/ds18x20/ds18x20.h"


static EventGroupHandle_t wifi_event_group;
static int retryNumber = 0;

#define WIFI_CONNECTED_BIT (1<<0)
#define WIFI_FAIL_BIT      (1<<1)
#define RETRYMAX 5

float temp = 0;
SemaphoreHandle_t temp_semaphore;
static TaskHandle_t temperature_task_handle = NULL;
static TaskHandle_t publish_task_handle = NULL;



static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	printf("Event dispatched from event loop base=%s, event_id=%d\n", event_base, event_id);
	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		        printf("got IP:%s", ip4addr_ntoa(&event->ip_info.ip));
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
		printf("WIFI Connection Failed; Reconnecting....\n");
	}
}

static void mqtt_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	switch(event_id){
	case MQTT_EVENT_CONNECTED:
		xTaskNotifyGive(publish_task_handle);
		printf("Connected\n");
		break;
	case MQTT_EVENT_DISCONNECTED:
		printf("Disconnected\n");
		break;
	case MQTT_EVENT_SUBSCRIBED:
		printf("Subscribed\n");
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		printf("UnSubscribed\n");
		break;
	case MQTT_EVENT_PUBLISHED:
		printf("Published\n");
		break;
	case MQTT_EVENT_DATA:
		printf("Data\n");
		break;
	case MQTT_EVENT_ERROR:
		printf("Error\n");
		break;
	case MQTT_EVENT_BEFORE_CONNECT:
		printf("Before Connection\n");
		break;
	default:
		printf("Other Command\n");
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
        xSemaphoreTake(temp_semaphore, portMAX_DELAY);
    	snprintf(temperature, sizeof(temperature), "%.4f", temp);
        xSemaphoreGive(temp_semaphore);
        esp_mqtt_client_publish(client, "sen", temperature, 0, 1, 0);
    	vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void measure_temperature(void * parameter){
    temp_semaphore = xSemaphoreCreateMutex();
	vTaskDelay(pdMS_TO_TICKS(2000));
	 ds18x20_addr_t ds18b20_address[1];
	 int sensor_count = 0;

	 while(sensor_count < 1){
		 sensor_count = ds18x20_scan_devices(18, ds18b20_address, 1);
		 printf("working\n");
		 vTaskDelay(pdMS_TO_TICKS(1000));
	 }
    	for(;;){
		    xSemaphoreTake(temp_semaphore, portMAX_DELAY);
    		esp_err_t error = ds18x20_measure_and_read(18, ds18b20_address[0] , &temp);
    	    xSemaphoreGive(temp_semaphore);
    		if(error == ESP_OK){
    		    xSemaphoreTake(temp_semaphore, portMAX_DELAY);
        		printf("%f\n", temp);
        	    xSemaphoreGive(temp_semaphore);
    		} else if(error == ESP_ERR_INVALID_RESPONSE){
    			printf("Temperature Sensor not Connected\n");
    		} else if(error == ESP_ERR_INVALID_CRC){
    			printf("Invalid CRC, Try Again\n");
    		} else {
    			printf("Unknown Error\n");
    		}
    		vTaskDelay(pdMS_TO_TICKS(1750));
    	}
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
            .ssid = "MySpectrumWiFic0-2G",
            .password = "bluebrain782"
        },
    };
    printf("hello\n");
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    EventBits_t eventBits;
    eventBits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if((eventBits & WIFI_CONNECTED_BIT) != 0){
    	printf("WIFI Connection Success\n");
        ESP_LOGI("System Info", "[APP] Startup..");
        ESP_LOGI("System Info", "[APP] Free memory: %d bytes", esp_get_free_heap_size());
        ESP_LOGI("System Info", "[APP] IDF version: %s", esp_get_idf_version());

    	xTaskCreatePinnedToCore(measure_temperature, "temperature_task", 2500, NULL, 2, &temperature_task_handle, 1);
    	xTaskCreatePinnedToCore(publish_data, "publish_task", 2500, NULL, 2, &publish_task_handle, 1);
    }
    else if((eventBits & WIFI_FAIL_BIT) != 0){
    	printf("WIFI Connection Failed\n");
    } else {
    	printf("Unexpected Event\n");
    }
}
