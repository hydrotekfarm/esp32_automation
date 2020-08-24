#include "ph_reading.h"

#include <esp_log.h>

#include "ph_sensor.h"
#include "sync_sensors.h"
#include "port_manager.h"
#include "task_manager.h"

void measure_ph(void *parameter) {				// pH Sensor Measurement Task
	const char *TAG = "PH_Task";

	ph_active = false;
	_ph = 0;
	ph_calibration = false;

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
