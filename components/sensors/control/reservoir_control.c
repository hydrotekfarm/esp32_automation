#include "reservoir_control.h"
#include <esp_log.h>
#include <esp_err.h>
#include <esp_system.h>
#include "ph_control.h"
#include "ec_control.h"
#include "ports.h"
#include "rtc.h"
#include "ec_reading.h"
#include "sync_sensors.h"
#include "control_settings_keys.h"
#include "control_task.h"
#include "sensor_control.h"

char *TAG = "RESERVOIR_CONTROL";

SemaphoreHandle_t float_switch_top_semaphore = NULL;
SemaphoreHandle_t float_switch_bottom_semaphore = NULL;

// ISR handler for top float switch
void IRAM_ATTR top_float_switch_isr_handler(void* arg) {
	gpio_isr_handler_remove(FLOAT_SWITCH_TOP_GPIO); // Remove ISR handler of top float switch in order to prevent multiple interrupts due to switch bounce
	xSemaphoreGiveFromISR(float_switch_top_semaphore, NULL); // Signal that interrupt occurred
}

// ISR handler for bottom float switch
void IRAM_ATTR bottom_float_switch_isr_handler(void* arg) {
	gpio_isr_handler_remove(FLOAT_SWITCH_BOTTOM_GPIO); // Remove ISR handler of bottom float switch in order to prevent multiple interrupts due to switch bounce
	xSemaphoreGiveFromISR(float_switch_bottom_semaphore, NULL); // Signal that interrupt occurred
}

// Setter for reservoir change flag
void set_reservoir_change_flag(bool active) {
	reservoir_change_flag = active;
}

// Drain reservoir using wireless sump pump
esp_err_t drain_tank() {
	bool logic_level = gpio_get_level(FLOAT_SWITCH_BOTTOM_GPIO); // Tank is empty when float switch reads 0 and vice versa
	if(logic_level == 0) { // Check if tank is already empty
		ESP_LOGI(TAG, "Tank is already empty");
		return ESP_OK;
	} else {
		ESP_LOGI(TAG, "Tank is not empty");
		float_switch_bottom_semaphore = xSemaphoreCreateBinary();
		gpio_set_intr_type(FLOAT_SWITCH_BOTTOM_GPIO, GPIO_INTR_NEGEDGE);	// Create interrupt that gets triggered on falling edge (1 -> 0)
		gpio_isr_handler_add(FLOAT_SWITCH_BOTTOM_GPIO, bottom_float_switch_isr_handler, NULL);
		ESP_LOGI(TAG, "drain power outlet on");
		water_out_rf_message.state = POWER_OUTLET_ON;
		xQueueSend(rf_transmitter_queue, &water_out_rf_message, portMAX_DELAY);	// Turn on water out power outlet

		// TODO Replace port max delay with approximate time it might take to drain reservoir
		bool is_complete = xSemaphoreTake(float_switch_bottom_semaphore, portMAX_DELAY); // Wait until interrupt gets triggered

		ESP_LOGI(TAG, "drain power outlet off");
		water_out_rf_message.state = POWER_OUTLET_OFF;
		xQueueSend(rf_transmitter_queue, &water_out_rf_message, portMAX_DELAY); // Turn off water out power outlet

		if(is_complete == pdFALSE) {
			// TODO Report system error as tank was not drained within expected time
			return PENDING;
		} else {
			ESP_LOGI(TAG, "Fully Drained");
			return ESP_OK;
		}
	}
}

// Fill up reservoir using wireless solenoid valve
esp_err_t fill_up_tank() {
	bool logic_level = gpio_get_level(FLOAT_SWITCH_TOP_GPIO); // Tank is empty when float switch reads 0 and vice versa
	if(logic_level == 1) { // Check if tank is already filled to the top
		ESP_LOGI(TAG, "Tank is already full");
		return ESP_OK;
	} else {
		ESP_LOGI(TAG, "Tank is empty");
		float_switch_top_semaphore = xSemaphoreCreateBinary();
		gpio_set_intr_type(FLOAT_SWITCH_TOP_GPIO, GPIO_INTR_POSEDGE); // Create interrupt that gets triggered on rising edge (0 -> 1)
		gpio_isr_handler_add(FLOAT_SWITCH_TOP_GPIO, top_float_switch_isr_handler, NULL);
		ESP_LOGI(TAG, "fillup power outlet on");
		water_in_rf_message.state = POWER_OUTLET_ON;
		xQueueSend(rf_transmitter_queue, &water_in_rf_message, portMAX_DELAY);	// Turn on water in power outlet

		// TODO Replace port max delay with approximate time it might take to fill reservoir
		bool is_complete = xSemaphoreTake(float_switch_top_semaphore, portMAX_DELAY); // Wait until interrupt gets triggered

		water_in_rf_message.state = POWER_OUTLET_OFF;
		ESP_LOGI(TAG, "fillup power outlet off");
		xQueueSend(rf_transmitter_queue, &water_in_rf_message, portMAX_DELAY); // Turn off water in power outlet

		if(is_complete == pdFALSE) {
			// TODO Report system error as tank was not filled within expected time
			return PENDING;
		} else {
			ESP_LOGI(TAG, "Filled to the top");
			return ESP_OK;
		}
	}
}

void check_water_level() {
	// Check if ph and ec is currently being altered
	bool ec_control = control_get_active(get_ec_control());
	bool ph_control = control_get_active(get_ph_control());

	if(!ec_control || !ph_control) {
		if(reservoir_change_flag) {
			esp_err_t error;
			error = drain_tank(); // Drain reservoir using sump pump
			if(error == PENDING) {
				// TODO Report System Error
				return;
			} else if(error != ESP_OK) {
				// TODO Report Unknown Error
				return;
			}

			error = fill_up_tank(); // Fill up reservoir using solenoid valve

			if(error == PENDING) {
				// TODO Report System Error
				return;
			} else if(error != ESP_OK) {
				// TODO Report Unknown Error
				return;
			}
			set_reservoir_change_flag(false); // Set Reservoir Change flag to false as process is successfully complete
			enable_timer(&dev, &water_pump_timer, water_pump_off_time); // TODO this has to be replaced
			return;
		}
		if(top_float_switch_trigger) {

			top_float_switch_trigger = false;
		}
	}
}
