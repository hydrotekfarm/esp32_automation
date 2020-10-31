#include "ec_reading.h"

#include <esp_log.h>
#include "string.h"
#include "ec_sensor.h"
#include "sync_sensors.h"
#include "task_priorities.h"
#include "ports.h"
#include "water_temp_reading.h"

const struct sensor* get_ec_sensor() { return &ec_sensor; }

void measure_ec(void *parameter) {				// EC Sensor Measurement Task
	const char *TAG = "EC_Task";

	init_sensor(&ec_sensor, true, true);
	dry_ec_calibration = false;

	ec_sensor_t dev;
	memset(&dev, 0, sizeof(ec_sensor_t));
	ESP_ERROR_CHECK(ec_init(&dev, 0, EC_ADDR_BASE, SDA_GPIO, SCL_GPIO)); // Initialize EC I2C communication

	for (;;) {
		if(sensor_calib_status(&ec_sensor)) { // Calibration Mode is activated
			ESP_LOGI(TAG, "Start Calibration");
			vTaskPrioritySet(sensor_get_task_handle(&ec_sensor), (configMAX_PRIORITIES - 1));	// Temporarily increase priority so that calibration can take place without interruption

			esp_err_t error = calibrate_ec(&dev); // Calibrate EC

			if (error != ESP_OK) {
				ESP_LOGE(TAG, "Calibration Failed: %d", error);
			} else {
				ESP_LOGI(TAG, "Calibration Success");
			}

			sensor_set_calib_status(&ec_sensor, false);	// Disable calibration mode, activate EC sensor and revert task back to regular priority
			vTaskPrioritySet(sensor_get_task_handle(&ec_sensor), EC_TASK_PRIORITY);
		} if(dry_ec_calibration) {
			ESP_LOGI(TAG, "Start Dry Calibration");
			vTaskPrioritySet(sensor_get_task_handle(&ec_sensor), (configMAX_PRIORITIES - 1));	// Temporarily increase priority so that calibration can take place without interruption

			esp_err_t error = calibrate_ec_dry(&dev); // Calibrate EC

			if (error != ESP_OK) {
				ESP_LOGE(TAG, "Calibration Failed: %d", error);
			} else {
				ESP_LOGI(TAG, "Calibration Success");
			}

			dry_ec_calibration = false;	// Disable calibration mode, activate EC sensor and revert task back to regular priority
			vTaskPrioritySet(sensor_get_task_handle(&ec_sensor), EC_TASK_PRIORITY);
		}	else {		// EC sensor is Active
			read_ec_with_temperature(&dev, 23, sensor_get_address_value(&ec_sensor));
			ESP_LOGI(TAG, "EC: %f", sensor_get_value(&ec_sensor));

			// Sync with other sensor tasks
			// Wait up to 10 seconds to let other tasks end
			xEventGroupSync(sensor_event_group, EC_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
		}
	}
}
