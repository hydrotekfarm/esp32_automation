#include "ec_reading.h"

#include <esp_log.h>

#include "ec_sensor.h"
#include "sync_sensors.h"
#include "task_priorities.h"
#include "ports.h"
#include "water_temp_reading.h"

void measure_ec(void *parameter) {				// EC Sensor Measurement Task
	const char *TAG = "EC_Task";

	_ec = 0;
	ec_calibration = false;

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
