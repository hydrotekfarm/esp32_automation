#include "control_task.h"
#include "reservoir_control.h"
#include "ph_control.h"
#include "ec_control.h"
#include "sync_sensors.h"
#include "ports.h"
#include "rf_transmitter.h"
#include "FreeRTOS/queue.h"
#include <esp_log.h>

void sensor_control (void *parameter) {
//	struct rf_message message;
//	message.rf_address_ptr = water_in_address;
//	while(true) {
//		ESP_LOGI("sensor control", "TURN ON");
//		message.state = POWER_OUTLET_ON;
//		xQueueSend(rf_transmitter_queue, &message, portMAX_DELAY);
//		vTaskDelay(pdMS_TO_TICKS(2000));
//
//		ESP_LOGI("sensor control", "TURN OFF");
//		message.state = POWER_OUTLET_OFF;
//		xQueueSend(rf_transmitter_queue, &message, portMAX_DELAY);
//		vTaskDelay(pdMS_TO_TICKS(2000));
//	}
	reset_sensor_checks(&ph_checks);
	reset_sensor_checks(&ec_checks);

	target_ph = 5;
	target_ec = 4;

	ph_dose_time = 5;
	ph_wait_time = 10 * 60;
	ec_dose_time = 10;
	ec_wait_time = 10 * 60;
	ec_nutrient_proportions[0] = 0.10;
	ec_nutrient_proportions[1] = 0.20;
	ec_nutrient_proportions[2] = 0.30;
	ec_nutrient_proportions[3] = 0.10;
	ec_nutrient_proportions[4] = 0.00;
	ec_nutrient_proportions[5] = 0.30;

	ec_pump_gpios[0] = EC_NUTRIENT_1_PUMP_GPIO;
	ec_pump_gpios[1] = EC_NUTRIENT_2_PUMP_GPIO;
	ec_pump_gpios[2] = EC_NUTRIENT_3_PUMP_GPIO;
	ec_pump_gpios[3] = EC_NUTRIENT_4_PUMP_GPIO;
	ec_pump_gpios[4] = EC_NUTRIENT_5_PUMP_GPIO;
	ec_pump_gpios[5] = EC_NUTRIENT_6_PUMP_GPIO;

	water_in_rf_message.rf_address_ptr = water_in_address;
	water_out_rf_message.rf_address_ptr = water_out_address;

	for(;;)  {
		// Check sensors
		if(reservoir_control_active) check_water_level();
		if(ph_control_active) check_ph();
		if(ec_control_active) check_ec();

		// Wait till next sensor readings
		vTaskDelay(pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}

void reset_sensor_checks(bool *sensor_checks) {
	for(int i = 0; i < sizeof(sensor_checks); i++) {
		sensor_checks[i] = false;
	}
}
