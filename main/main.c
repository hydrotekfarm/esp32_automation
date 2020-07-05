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
#include "ds3231.h"
#include "rf_transmission.h"

static const char *_TAG = "Main";

// WiFi Coordination with Event Group
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT (1<<0)
#define WIFI_FAIL_BIT      (1<<1)

// Sensor Task Coordination with Event Group
static EventGroupHandle_t sensor_event_group;
#define DELAY_BIT		    (1<<0)
#define WATER_TEMPERATURE_BIT	(1<<1)
#define EC_BIT 	        (1<<2)
#define PH_BIT		    (1<<3)
#define ULTRASONIC_BIT    (1<<4)

// Core 0 Task Priorities
#define TIMER_TASK_PRIORITY 1
#define MQTT_PUBLISH_TASK_PRIORITY 2

// Core 1 Task Priorities
#define ULTRASONIC_TASK_PRIORITY 1
#define PH_TASK_PRIORITY 2
#define EC_TASK_PRIORITY 3
#define WATER_TEMPERATURE_TASK_PRIORITY 4
#define SYNC_TASK_PRIORITY 5

#define MAX_DISTANCE_CM 500

// GPIO and ADC Ports
#define ULTRASONIC_TRIGGER_GPIO 18		// GPIO 18
#define ULTRASONIC_ECHO_GPIO 17			// GPIO 17
#define TEMPERATURE_SENSOR_GPIO 19		// GPIO 19
#define RTC_SCL_GPIO 22                 // GPIO 22
#define RTC_SDA_GPIO 21                 // GPIO 21
#define RF_TRANSMITTER_GPIO 32			// GPIO 32
#define EC_SENSOR_GPIO ADC_CHANNEL_0    // GPIO 36
#define PH_SENSOR_GPIO ADC_CHANNEL_3    // GPIO 39

#define SENSOR_MEASUREMENT_PERIOD 10000 // Measuring increment time in ms
#define SENSOR_DISABLED_PERIOD SENSOR_MEASUREMENT_PERIOD / 2 // Disabled increment is half of measure period so task always finishes on time

#define RETRYMAX 5 // WiFi MAX Reconnection Attempts
#define DEFAULT_VREF 1100  // ADC Voltage Reference

static int retryNumber = 0;  // WiFi Reconnection Attempts

static esp_adc_cal_characteristics_t *adc_chars;  // ADC 1 Configuration Settings

// IDs
static char growroom_id[] = "Grow Room 1";
static char system_id[] = "System 1";

// Sensor Measurement Variables
static float _water_temp = 0;
static float _ec = 0;
static float _distance = 0;
static float _ph = 0;

// RTC Components
i2c_dev_t dev;

// Timers
struct timer water_pump_timer;

static const uint32_t timer_urgent_delay = 10;
static const uint32_t timer_regular_delay = 50;

// Water pump timings
static uint32_t water_pump_on_time = 5;
static uint32_t water_pump_off_time  = 10;

// Task Handles
static TaskHandle_t water_temperature_task_handle = NULL;
static TaskHandle_t ec_task_handle = NULL;
static TaskHandle_t ph_task_handle = NULL;
static TaskHandle_t ultrasonic_task_handle = NULL;
static TaskHandle_t sync_task_handle = NULL;
static TaskHandle_t publish_task_handle = NULL;
static TaskHandle_t timer_task_handle = NULL;

// Sensor Active Status
static bool water_temperature_active = false;
static bool ec_active = false;
static bool ph_active = true;
static bool ultrasonic_active = true;

static uint32_t sensor_sync_bits;

// Sensor Calibration Status
static bool ec_calibration = false;
static bool ph_calibration = false;

static void restart_esp32() { // Restart ESP32
	fflush(stdout);
	esp_restart();
}

static void init_rtc() { // Init RTC
	ESP_ERROR_CHECK(i2cdev_init());
	memset(&dev, 0, sizeof(i2c_dev_t));
	ESP_ERROR_CHECK(ds3231_init_desc(&dev, 0, RTC_SDA_GPIO, RTC_SCL_GPIO));
}
static void set_time() { // Set current time to some date
	// TODO Have user input for time so actual time is set
	struct tm time;

	time.tm_year = 120; // Years since 1900
	time.tm_mon = 5; // 0-11
	time.tm_mday = 29; // day of month
	time.tm_hour = 13; // 0-24
	time.tm_min = 14;
	time.tm_sec = 0;

	ESP_ERROR_CHECK(ds3231_set_time(&dev, &time));
}

static void check_rtc_reset() {
	// Get current time
	struct tm time;
	ds3231_get_time(&dev, &time);

	// If year is less than 2020 (RTC was reset), set time again
	if(time.tm_year < 120) set_time();
}

static void get_date_time(struct tm *time) {
	// Get current time and set it to return var
	ds3231_get_time(&dev, &(*time));

	// If year is less than 2020 (RTC was reset), set time again and set new time to return var
	if(time->tm_year < 120) {
		set_time();
		ds3231_get_time(&dev, &(*time));
	}
}

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
//		if (true) {
//			ec_calibration = true;
//		}
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

	// Set broker configuration
	esp_mqtt_client_config_t mqtt_cfg = { .host = "70.94.9.135", .port = 1883 };

	// Create and initialize MQTT client
	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler,
			client);
	esp_mqtt_client_start(client);
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
	for (;;) {
		// Create and structure topic for publishing data through MQTT
		char *topic = NULL;
		create_str(&topic, growroom_id);
		append_str(&topic, "/");
		append_str(&topic, "systems/");
		append_str(&topic, system_id);

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
		if(water_temperature_active) {
			add_entry(&data, &first, "water temp", _water_temp);
		}

		if(ec_active) {
			add_entry(&data, &first, "ec", _ec);
		}

		if(ph_active) {
			add_entry(&data, &first, "ph", _ph);
		}

		if(ultrasonic_active) {
			add_entry(&data, &first, "distance", _distance);
		}

		// Add closing tag
		append_str(&data, "]}");

		// Publish data to MQTT broker using topic and data
		esp_mqtt_client_publish(client, topic, data, 0, 1, 0);

		ESP_LOGI(TAG, "Topic: %s", topic);
		ESP_LOGI(TAG, "Message: %s", data);

		// Free allocated memory
		free(data);
		data = NULL;

		// Publish data every 20 seconds
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}

// Trigger method for water pump timer
void water_pump() {
	const char *TAG = "Water Pump";

	if(water_pump_timer.duration == water_pump_on_time) {
		// TODO Turn water pump off

		ESP_LOGI(TAG, "Turning pump off");
		enable_timer(&dev, &water_pump_timer, water_pump_off_time);
	} else if(water_pump_timer.duration == water_pump_off_time) {
		// TODO Turn water pump on

		ESP_LOGI(TAG, "Turning  pump on");
		enable_timer(&dev, &water_pump_timer, water_pump_on_time);
	}
}


static void manage_timers(void *parameter) {
	const char *TAG = "TIMER_TASK";

	init_timer(&water_pump_timer, &water_pump, false, false);
	enable_timer(&dev, &water_pump_timer, water_pump_on_time);

	ESP_LOGI(TAG, "Timers initialized");

	for(;;) {
		time_t unix_time;
		get_unix_time(&dev, &unix_time);


		check_timer(&dev, &water_pump_timer, unix_time);

		bool urgent = (water_pump_timer.active && water_pump_timer.high_priority);

		vTaskPrioritySet(timer_task_handle, urgent ? (configMAX_PRIORITIES - 1) : TIMER_TASK_PRIORITY);
		vTaskDelay(pdMS_TO_TICKS(urgent ? timer_urgent_delay : timer_regular_delay));
	}
}

void measure_water_temperature(void *parameter) {		// Water Temperature Measurement Task
	const char *TAG = "Temperature_Task";
	ds18x20_addr_t ds18b20_address[1];

	// Scan and setup sensor
	uint32_t sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO,
			ds18b20_address, 1);

	sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO,
			ds18b20_address, 1);
	vTaskDelay(pdMS_TO_TICKS(1000));

	if(sensor_count < 1 && water_temperature_active) ESP_LOGE(TAG, "Sensor Not Found");

	for (;;) {
		// Perform Temperature Calculation and Read Temperature; vTaskDelay in the source code of this function
		esp_err_t error = ds18x20_measure_and_read(TEMPERATURE_SENSOR_GPIO,
				ds18b20_address[0], &_water_temp);
		// Error Management
		if (error == ESP_OK) {
			ESP_LOGI(TAG, "temperature: %f\n", _water_temp);
		} else if (error == ESP_ERR_INVALID_RESPONSE) {
			ESP_LOGE(TAG, "Temperature Sensor Not Connected\n");
		} else if (error == ESP_ERR_INVALID_CRC) {
			ESP_LOGE(TAG, "Invalid CRC, Try Again\n");
		} else {
			ESP_LOGE(TAG, "Unknown Error\n");
		}

		// Sync with other sensor tasks
		// Wait up to 10 seconds to let other tasks end
		xEventGroupSync(sensor_event_group, WATER_TEMPERATURE_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}

void measure_ec(void *parameter) {				// EC Sensor Measurement Task
	const char *TAG = "EC_Task";
	ec_begin();		// Setup EC sensor and get calibrated values stored in NVS

	for (;;) {
		if(ec_calibration) { // Calibration Mode is activated
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
				_ec = readEC(voltage, _water_temp);
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
		} else {		// EC sensor is Active
			uint32_t adc_reading = 0;
			// Get a Raw Voltage Reading
			for (int i = 0; i < 64; i++) {
				adc_reading += adc1_get_raw(EC_SENSOR_GPIO);
			}
			adc_reading /= 64;
			float voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
			_ec = readEC(voltage, _water_temp);	// Calculate EC from voltage reading
			ESP_LOGI(TAG, "EC: %f\n", _ec);

			// Sync with other sensor tasks
			// Wait up to 10 seconds to let other tasks end
			xEventGroupSync(sensor_event_group, EC_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
		}
	}
}

void measure_ph(void *parameter) {				// pH Sensor Measurement Task
	const char *TAG = "PH_Task";
	ph_begin();	// Setup pH sensor and get calibrated values stored in NVS

	for (;;) {
		if(ph_calibration) {
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
		} else {	// pH sensor is Active
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
			// Wait up to 10 seconds to let other tasks end
			xEventGroupSync(sensor_event_group, PH_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
		}
	}
}

void measure_distance(void *parameter) {		// Ultrasonic Sensor Distance Measurement Task
	const char *TAG = "ULTRASONIC_TASK";

	// Setting sensor ports and initializing
	ultrasonic_sensor_t sensor = { .trigger_pin = ULTRASONIC_TRIGGER_GPIO,
			.echo_pin = ULTRASONIC_ECHO_GPIO };

	ultrasonic_init(&sensor);

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
			ESP_LOGI(TAG, "Distance: %f cm\n", distance);
			_distance = distance;
		}

		// Sync with other sensor tasks
		// Wait up to 10 seconds to let other tasks end
		xEventGroupSync(sensor_event_group, ULTRASONIC_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}

void set_sensor_sync_bits(uint32_t *bits) {
	//*bits = 24;
	*bits = DELAY_BIT | (water_temperature_active ? WATER_TEMPERATURE_BIT : 0) | (ec_active ? EC_BIT : 0) | (ph_active ? PH_BIT : 0) | (ultrasonic_active ? ULTRASONIC_BIT : 0);
}

void sync_task(void *parameter) {				// Sensor Synchronization Task
	const char *TAG = "Sync_Task";
	EventBits_t returnBits;
	for (;;) {
		// Provide a minimum amount of time before event group round resets
		vTaskDelay(pdMS_TO_TICKS(10000));
		returnBits = xEventGroupSync(sensor_event_group, DELAY_BIT, sensor_sync_bits, pdMS_TO_TICKS(10000));
		// Check Whether all tasks were completed on time
		if ((returnBits & sensor_sync_bits) == sensor_sync_bits) {
			ESP_LOGI(TAG, "Completed");
		} else {
			ESP_LOGE(TAG, "Failed to Complete On Time");
		}
	}
}

void send_rf_transmission(){
	// Setup Transmission Protocol
	struct binary_bits low_bit = {3, 1};
	struct binary_bits sync_bit = {31, 1};
	struct binary_bits high_bit = {1, 3};
	configure_protocol(172, 10, 32, low_bit, high_bit, sync_bit);

	// Start controlling outlets
	send_message("000101000101010100110011"); // Binary Code to turn on Power Outlet 1
	vTaskDelay(pdMS_TO_TICKS(5000));
	send_message("000101000101010100111100"); // Binary Code to turn off Power Outlet 1
	vTaskDelay(pdMS_TO_TICKS(5000));
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

	gpio_set_direction(32, GPIO_MODE_OUTPUT);
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
				ESP_LOGE(_TAG,
						"EFUSE_VREF not supported, use a different ESP 32 board");
			}

			// Set all sync bits var
			set_sensor_sync_bits(&sensor_sync_bits);

			// Init rtc and check if time needs to be set
			init_rtc();
			check_rtc_reset();

			// Create core 0 tasks
			xTaskCreatePinnedToCore(manage_timers, "timer_task", 2500, NULL, TIMER_TASK_PRIORITY, &timer_task_handle, 0);
			xTaskCreatePinnedToCore(publish_data, "publish_task", 2500, NULL, MQTT_PUBLISH_TASK_PRIORITY, &publish_task_handle, 0);

			// Create core 1 tasks
			if(water_temperature_active) xTaskCreatePinnedToCore(measure_water_temperature, "temperature_task", 2500, NULL, WATER_TEMPERATURE_TASK_PRIORITY, &water_temperature_task_handle, 1);
			if(ec_active) xTaskCreatePinnedToCore(measure_ec, "ec_task", 2500, NULL, EC_TASK_PRIORITY, &ec_task_handle, 1);
			if(ph_active) xTaskCreatePinnedToCore(measure_ph, "ph_task", 2500, NULL, PH_TASK_PRIORITY, &ph_task_handle, 1);
			if(ultrasonic_active) xTaskCreatePinnedToCore(measure_distance, "ultrasonic_task", 2500, NULL, ULTRASONIC_TASK_PRIORITY, &ultrasonic_task_handle, 1);
			xTaskCreatePinnedToCore(sync_task, "sync_task", 2500, NULL, SYNC_TASK_PRIORITY, &sync_task_handle, 1);

		} else if ((eventBits & WIFI_FAIL_BIT) != 0) {
			ESP_LOGE("", "WIFI Connection Failed\n");
		} else {
			ESP_LOGE("", "Unexpected Event\n");
		}
	}
