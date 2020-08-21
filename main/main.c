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

// From  boot
#include "boot.h"

// From sensor_libs
#include "ec_sensor.h"
#include "ds18x20.h"
#include "ph_sensor.h"
#include "ultrasonic.h"
#include "ds3231.h"
#include "rf_transmission.h"

// Sensor Task Coordination with Event Group
static EventGroupHandle_t sensor_event_group;
#define DELAY_BIT		    (1<<0)
#define WATER_TEMPERATURE_BIT	(1<<1)
#define EC_BIT 	        (1<<2)
#define PH_BIT		    (1<<3)
#define ULTRASONIC_BIT    (1<<4)

#define MAX_DISTANCE_CM 500

// Sensor Measurement Variables
static float _water_temp = 0;
static float _ec = 0;
static float _distance = 0;
static float _ph = 0;

// pH control
static float target_ph = 5;
static float ph_margin_error = 0.3;
static bool ph_checks[6] = {false, false, false, false, false, false};
static float ph_dose_time = 5;
static float ph_wait_time = 10 * 60;

// ec control
static float target_ec = 4;
static float ec_margin_error = 0.5;
static bool ec_checks[6] = {false, false, false, false, false, false};
static float ec_dose_time = 10;
static float ec_wait_time = 10 * 60;
static uint32_t ec_num_pumps = 6;
static uint32_t ec_nutrient_index = 0;
static float ec_nutrient_proportions[6] = {0.10, 0.20, 0.30, 0.10, 0, 0.30};
static uint32_t ec_pump_gpios[6] = {EC_NUTRIENT_1_PUMP_GPIO, EC_NUTRIENT_2_PUMP_GPIO, EC_NUTRIENT_3_PUMP_GPIO, EC_NUTRIENT_4_PUMP_GPIO, EC_NUTRIENT_5_PUMP_GPIO, EC_NUTRIENT_6_PUMP_GPIO};

// RTC Components
i2c_dev_t dev;

// Timer and alarm periods
static const uint32_t timer_alarm_urgent_delay = 10;
static const uint32_t timer_alarm_regular_delay = 50;

// Timers
struct timer water_pump_timer;
struct timer ph_dose_timer;
struct timer ph_wait_timer;
struct timer ec_dose_timer;
struct timer ec_wait_timer;

// Alarms
struct alarm lights_on_alarm;
struct alarm lights_off_alarm;

// Water pump timings
static uint32_t water_pump_on_time = 15 * 60;
static uint32_t water_pump_off_time  = 45 * 60;

// Lights
static uint32_t lights_on_hour = 21;
static uint32_t lights_on_min = 0;
static uint32_t lights_off_hour  = 6;
static uint32_t lights_off_min = 0;

// Sensor Active Status
static bool water_temperature_active = false;
static bool ec_active = true;
static bool ph_active = true;
static bool ultrasonic_active = false;

static uint32_t sensor_sync_bits;

// Sensor Calibration Status
static bool ec_calibration = false;
static bool ph_calibration = false;

static void init_rtc() { // Init RTC
	memset(&dev, 0, sizeof(i2c_dev_t));
	ESP_ERROR_CHECK(ds3231_init_desc(&dev, 0, RTC_SDA_GPIO, RTC_SCL_GPIO));
}
static void set_time() { // Set current time to some date
	// TODO Have user input for time so actual time is set
	struct tm time;

	time.tm_year = 120; // Years since 1900
	time.tm_mon = 6; // 0-11
	time.tm_mday = 7; // day of month
	time.tm_hour = 9; // 0-24
	time.tm_min = 59;
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



// Trigger method for water pump timer
void water_pump() {
	const char *TAG = "Water Pump";

	// Check if pump was just on
	if(water_pump_timer.duration == water_pump_on_time) {
		// TODO Turn water pump off

		// Start pump off timer
		ESP_LOGI(TAG, "Turning pump off");
		enable_timer(&dev, &water_pump_timer, water_pump_off_time);

		// Pump was just off
	} else if(water_pump_timer.duration == water_pump_off_time) {
		// TODO Turn water pump on

		// Start pump on  timer
		ESP_LOGI(TAG, "Turning  pump on");
		enable_timer(&dev, &water_pump_timer, water_pump_on_time);
	}
}

// Turn lights on
void lights_on() {
	// TODO Turn lights on
	ESP_LOGI("", "Turning lights on");
}

// Turn lights off
void lights_off() {
	// TODO Turn lights off
	ESP_LOGI("", "Turning lights off");
}

void get_lights_times(struct tm *lights_on_time, struct tm *lights_off_time) {
	// Get current date time
	struct tm current_date_time;
	ds3231_get_time(&dev, &current_date_time);

	// Set alarm times
	lights_on_time->tm_year = current_date_time.tm_year;
	lights_on_time->tm_mon = current_date_time.tm_mon;
	lights_on_time->tm_mday = current_date_time.tm_mday;
	lights_on_time->tm_hour = lights_on_hour;
	lights_on_time->tm_min  = lights_on_min;
	lights_on_time->tm_sec = 0;

	lights_off_time->tm_year = current_date_time.tm_year;
	lights_off_time->tm_mon = current_date_time.tm_mon;
	lights_off_time->tm_mday = current_date_time.tm_mday;
	lights_off_time->tm_hour = lights_off_hour;
	lights_off_time->tm_min  = lights_off_min;
	lights_off_time->tm_sec = 0;
}

void ph_up_pump() { // Turn ph up pump on
	gpio_set_level(PH_UP_PUMP_GPIO, 1);
	ESP_LOGI("", "pH up pump on");

	// Enable dose timer
	enable_timer(&dev, &ph_dose_timer, ph_dose_time);
}

void ph_down_pump() { // Turn ph down pump on
	gpio_set_level(PH_DOWN_PUMP_GPIO, 1);
	ESP_LOGI("", "pH down pump on");

	// Enable dose timer
	enable_timer(&dev, &ph_dose_timer, ph_dose_time);
}

void ph_pump_off() { // Turn ph pumps off
	gpio_set_level(PH_UP_PUMP_GPIO, 0);
	gpio_set_level(PH_DOWN_PUMP_GPIO, 0);
	ESP_LOGI("", "pH pumps off");

	// Enable wait timer
	enable_timer(&dev, &ph_wait_timer, ph_wait_time - (sizeof(ph_checks) * (SENSOR_MEASUREMENT_PERIOD  / 1000))); // Offset wait time based on amount of checks and check duration
}

void reset_sensor_checks(bool *sensor_checks) { // Reset sensor check vars
	for(int i = 0; i < sizeof(sensor_checks); i++) {
		sensor_checks[i] = false;
	}
}

void check_ph() { // Check ph
	char *TAG = "PH_CONTROL";

	// Check if ph and ec is currently being altered
	bool ph_control = ph_dose_timer.active || ph_wait_timer.active;
	bool ec_control = ec_dose_timer.active || ec_wait_timer.active;

	// Only proceed if ph and ec are not being altered
	if(!ph_control && !ec_control) {
		// Check if ph is too low
		if(_ph < target_ph - ph_margin_error) {
			// Check if all checks are complete
			if(ph_checks[sizeof(ph_checks) - 1]) {
				// Turn pump on and reset checks
				ph_up_pump();
				reset_sensor_checks(ph_checks);
			} else {
				// Iterate through checks
				for(int i = 0; i < sizeof(ph_checks); i++) {
					// Once false check is reached, set it to true and leave loop
					if(!ph_checks[i]) {
						ph_checks[i] = true;
						ESP_LOGI(TAG, "PH check %d done", i+1);
						break;
					}
				}
			}
			// Check if ph is too high
		} else if(_ph > target_ph + ph_margin_error) {
			// Check if ph checks are complete
			if(ph_checks[sizeof(ph_checks) - 1]) {
				// Turn pump on and reset checks
				ph_down_pump();
				reset_sensor_checks(ph_checks);
			} else {
				// Iterate through checks
				for(int i = 0; i < sizeof(ph_checks); i++) {
					// Once false check is reached, set it to true and leave loop
					if(!ph_checks[i]) {
						ph_checks[i] = true;
						ESP_LOGI(TAG, "PH check %d done", i+1);
						break;
					}
				}
			}
		} else {
			// Reset checks if ph is good
			reset_sensor_checks(ph_checks);
		}
		// Reset sensor checks if ec is active
	} else if(ec_active) {
		reset_sensor_checks(ph_checks);
	}
}

void ec_dose() {
	// Check if last nutrient was pumped
	if(ec_nutrient_index == ec_num_pumps) {
		// Turn off last pump
		gpio_set_level(ec_pump_gpios[ec_nutrient_index - 1], 0);

		// Enable wait timer and reset nutrient index
		enable_timer(&dev, &ec_wait_timer, ec_wait_time - (sizeof(ec_checks) * (SENSOR_MEASUREMENT_PERIOD  / 1000))); // Offset timer based on check durations
		ec_nutrient_index = 0;
		ESP_LOGI("", "EC dosing done");

		// Still nutrients left
	} else {
		// Turn off last pump as long as this isn't first pump and turn on next pump
		if(ec_nutrient_index != 0) gpio_set_level(ec_pump_gpios[ec_nutrient_index - 1], 0);
		gpio_set_level(ec_pump_gpios[ec_nutrient_index], 1);

		// Enable dose timer based on nutrient proportion
		enable_timer(&dev, &ec_dose_timer, ec_dose_time * ec_nutrient_proportions[ec_nutrient_index]);
		ESP_LOGI("", "Dosing nutrient %d for %.2f seconds", ec_nutrient_index  + 1, ec_dose_time * ec_nutrient_proportions[ec_nutrient_index]);
		ec_nutrient_index++;
	}
}

void check_ec() {
	char *TAG = "EC_CONTROL";

	// Check if ph and ec is currently being altered
	bool ec_control = ec_dose_timer.active || ec_wait_timer.active;
	bool ph_control = ph_dose_timer.active || ph_wait_timer.active;

	if(!ec_control && !ph_control) {
		if(_ec < target_ec - ec_margin_error) {
			// Check if all checks are complete
			if(ec_checks[sizeof(ec_checks) - 1]) {
				ec_dose();
				reset_sensor_checks(ec_checks);
			} else {
				// Iterate through checks
				for(int i = 0; i < sizeof(ec_checks); i++) {
					// Once false check is reached, set it to true and leave loop
					if(!ec_checks[i]) {
						ec_checks[i] = true;
						ESP_LOGI(TAG, "EC check %d done", i+1);
						break;
					}
				}
			}
		} else if(_ec > target_ec + ec_margin_error) {
			// Check if all checks are complete
			if(ec_checks[sizeof(ec_checks) - 1]) {
				// TODO dilute ec with  water
				reset_sensor_checks(ec_checks);
			} else {
				// Iterate through checks
				for(int i = 0; i < sizeof(ec_checks); i++) {
					// Once false check is reached, set it to true and leave loop
					if(!ec_checks[i]) {
						ec_checks[i] = true;
						ESP_LOGI(TAG, "EC check %d done", i+1);
						break;
					}
				}
			}
		}
		// Reset sensor checks if ph is active
	} else if(ph_control) {
		reset_sensor_checks(ec_checks);
	}
}

void sensor_control (void *parameter) { // Sensor Control Task
	for(;;)  {
		// Check sensors
		check_ph();
		check_ec();

		// Wait till next sensor readings
		vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
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

void do_nothing() {}

static void manage_timers_alarms(void *parameter) {
	const char *TAG = "TIMER_TASK";

	// Initialize timers
	init_timer(&water_pump_timer, &water_pump, false, false);
	init_timer(&ph_dose_timer, &ph_pump_off, false, true);
	init_timer(&ph_wait_timer, &do_nothing, false, false);
	init_timer(&ec_dose_timer, &ec_dose, false, true);
	init_timer(&ec_wait_timer, &do_nothing, false, false);

	// Get alarm times
	struct tm lights_on_time;
	struct tm lights_off_time;
	get_lights_times(&lights_on_time, &lights_off_time);

	// Initialize alarms
	init_alarm(&lights_on_alarm, &lights_on, true, false);
	init_alarm(&lights_off_alarm, &lights_off, true, false);

	ESP_LOGI(TAG, "Timers initialized");

	for(;;) {
		// Get current unix time
		time_t unix_time;
		get_unix_time(&dev, &unix_time);

		// Check if timers are done
		check_timer(&dev, &water_pump_timer, unix_time);
		check_timer(&dev, &ph_dose_timer, unix_time);
		check_timer(&dev, &ph_wait_timer, unix_time);
		check_timer(&dev, &ec_dose_timer, unix_time);
		check_timer(&dev, &ec_wait_timer, unix_time);

		// Check if alarms are done
		check_alarm(&dev, &lights_on_alarm, unix_time);
		check_alarm(&dev, &lights_off_alarm, unix_time);

		// Check if any timer or alarm is urgent
		bool urgent = (water_pump_timer.active && water_pump_timer.high_priority) || (ph_dose_timer.active && ph_dose_timer.high_priority) || (ph_wait_timer.active && ph_wait_timer.high_priority) || (ec_dose_timer.active && ec_dose_timer.high_priority) || (ec_wait_timer.active && ec_wait_timer.high_priority) || (lights_on_alarm.alarm_timer.active && lights_on_alarm.alarm_timer.high_priority) || (lights_off_alarm.alarm_timer.active && lights_off_alarm.alarm_timer.high_priority);

		// Set priority and delay based on urgency of timers and alarms
		vTaskPrioritySet(timer_alarm_task_handle, urgent ? (configMAX_PRIORITIES - 1) : TIMER_ALARM_TASK_PRIORITY);
		vTaskDelay(pdMS_TO_TICKS(urgent ? timer_alarm_urgent_delay : timer_alarm_regular_delay));
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

void app_main(void) {							// Main Method
	boot_sequence();
}
