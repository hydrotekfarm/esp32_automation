#include "ph_reading.h"
#include "grow_manager.h"
#include <esp_log.h>
#include <string.h>
#include "sync_sensors.h"
#include "task_priorities.h"
#include "ports.h"
#include "water_temp_reading.h"

struct sensor *get_ph_sensor() { return &ph_sensor; }

ph_sensor_t *get_ph_dev() { return &ph_dev; }

bool get_is_ph_activated() { return is_ph_activated; }

void set_is_ph_activated(bool is_active) { is_ph_activated = is_active; }


void measure_ph(void *parameter) {		// pH Sensor Measurement Task

	const char *TAG = "PH_Task";

	init_sensor(&ph_sensor, "ph", true, false);

	memset(&ph_dev, 0, sizeof(ph_sensor_t));

	ESP_ERROR_CHECK(ph_init(&ph_dev, 0, PH_ADDR_BASE, SDA_GPIO, SCL_GPIO)); // Initialize PH I2C communication


	is_ph_activated = false;

	ESP_ERROR_CHECK_WITHOUT_ABORT(activate_ph(&ph_dev));

	is_ph_activated = true;

	vTaskDelay(pdMS_TO_TICKS(1000));
	for (;;) {
		if(sensor_calib_status(&ph_sensor)) {
			ESP_LOGE(TAG, "PH Calibration Started");
            calibrate_sensor(&ph_sensor, &calibrate_ph, &ph_dev);
            sensor_set_calib_status(&ph_sensor, false); // Disable calibration mode, activate pH sensor and revert task back to regular priority
            if (!get_is_grow_active()) {
				vTaskSuspend(*sensor_get_task_handle(get_water_temp_sensor()));
                vTaskSuspend(*sensor_get_task_handle(&ph_sensor));
                ESP_LOGE(TAG, "PH and Water Temp task suspended");
            }
            ESP_LOGE(TAG, "PH Calibration Completed");
		} else {
			if (!get_is_ph_activated()) {
				ESP_ERROR_CHECK_WITHOUT_ABORT(activate_ph(&ph_dev));
				is_ph_activated = true;
			}
			read_ph_with_temperature(&ph_dev, sensor_get_value(get_water_temp_sensor()), sensor_get_address_value(&ph_sensor));
			ESP_LOGI(TAG, "PH: %f", sensor_get_value(&ph_sensor));
			// Sync with other sensor tasks and wait up to 10 seconds to let other tasks end
			xEventGroupSync(sensor_event_group, PH_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
		}
	}
}



