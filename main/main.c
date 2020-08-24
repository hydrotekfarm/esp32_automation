#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <lwip/sockets.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>
#include <stdbool.h>

#include "boot.h"

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

void do_nothing() {}



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
