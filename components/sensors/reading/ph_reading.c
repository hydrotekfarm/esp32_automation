#include "ph_reading.h"
#include "grow_manager.h"
#include <esp_log.h>
#include <string.h>
#include "ph_sensor.h"
#include "sync_sensors.h"
#include "task_priorities.h"
#include "ports.h"

struct sensor* get_ph_sensor() { return &ph_sensor; }

void measure_ph(void *parameter) {		// pH Sensor Measurement Task
	const char *TAG = "PH_Task";

	init_sensor(&ph_sensor, "ph", true, false);

	ph_sensor_t dev;
	memset(&dev, 0, sizeof(ph_sensor_t));

	ESP_ERROR_CHECK(ph_init(&dev, 0, PH_ADDR_BASE, SDA_GPIO, SCL_GPIO)); // Initialize PH I2C communication

	ESP_ERROR_CHECK(activate_ph(&dev));

	vTaskDelay(pdMS_TO_TICKS(1000));
	for (;;) {
		if(sensor_calib_status(&ph_sensor)) {
			ESP_LOGE(TAG, "PH Calibration Started");
			calibrate_sensor(&ph_sensor, &calibrate_ph, &dev);
			sensor_set_calib_status(&ph_sensor, false); // Disable calibration mode, activate pH sensor and revert task back to regular priority
			if (!is_grow_active) {
				vTaskSuspend(*sensor_get_task_handle(&ph_sensor));
				ESP_LOGE(TAG, "PH task suspended");
			}
			ESP_LOGE(TAG, "PH Calibration Completed");
		} else {
			read_ph_with_temperature(&dev, 25, sensor_get_address_value(&ph_sensor));
			ESP_LOGI(TAG, "ph: %f", sensor_get_value(&ph_sensor));

			// Sync with other sensor tasks and wait up to 10 seconds to let other tasks end
			//xEventGroupSync(sensor_event_group, PH_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
		}
	}
}
