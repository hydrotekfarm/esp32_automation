#include "ph_reading.h"
#include <esp_log.h>
#include <string.h>
#include "ph_sensor.h"
#include "sync_sensors.h"
#include "task_priorities.h"
#include "ports.h"

void measure_ph(void *parameter) {		// pH Sensor Measurement Task
	const char *TAG = "PH_Task";

	_ph = 0;
	ph_calibration = false;
	ph_sensor_t dev;
	memset(&dev, 0, sizeof(ph_sensor_t));

	ESP_ERROR_CHECK(ph_init(&dev, 0, PH_ADDR_BASE, SDA_GPIO, SCL_GPIO)); // Initialize PH I2C communication

	for (;;) {
		if(ph_calibration) {
			ESP_LOGI(TAG, "Start Calibration");
			vTaskPrioritySet(ph_task_handle, (configMAX_PRIORITIES - 1));	// Temporarily increase priority so that calibration can take place without interruption

			esp_err_t err = calibrate_ph(&dev, 25); // Calibrate PH using 25 degrees as Temperature
			if (err != ESP_OK) {
				ESP_LOGE(TAG, "Calibration Failed: %d", err);
			} else {
				ESP_LOGI(TAG, "Calibration Success");
			}

			ph_calibration = false; // Disable calibration mode, activate pH sensor and revert task back to regular priority
			vTaskPrioritySet(ph_task_handle, PH_TASK_PRIORITY);
		} else {
			read_ph_with_temperature(&dev, 25, &_ph);
			ESP_LOGI(TAG, "ph: %f", _ph);

			// Sync with other sensor tasks and wait up to 10 seconds to let other tasks end
			xEventGroupSync(sensor_event_group, PH_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
		}
	}
}
