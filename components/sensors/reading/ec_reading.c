#include "ec_reading.h"
#include "grow_manager.h"
#include <esp_log.h>
#include "string.h"
#include "ec_sensor.h"
#include "sync_sensors.h"
#include "task_priorities.h"
#include "ports.h"
#include "water_temp_reading.h"

struct sensor* get_ec_sensor() { return &ec_sensor; }

void measure_ec(void *parameter) {				// EC Sensor Measurement Task
	const char *TAG = "EC_Task";

	init_sensor(&ec_sensor, "ec", true, false);
	dry_calib = false;

	ec_sensor_t dev;
	memset(&dev, 0, sizeof(ec_sensor_t));
	ESP_ERROR_CHECK(ec_init(&dev, 0, EC_ADDR_BASE, SDA_GPIO, SCL_GPIO)); // Initialize EC I2C communication

	ESP_ERROR_CHECK(activate_ec(&dev));

	for (;;) {
		if(sensor_calib_status(&ec_sensor)) { // Calibration Mode is activated
			ESP_LOGI(TAG, "EC Wet Calibration Started");
            calibrate_sensor(&ec_sensor, &calibrate_ec, &dev);
            sensor_set_calib_status(&ec_sensor, false);
            if (!get_is_grow_active()) {
                ESP_LOGI(TAG, "EC task Suspended");
                vTaskSuspend(*sensor_get_task_handle(&ec_sensor));
            }
            ESP_LOGI(TAG, "EC Wet Calibration Completed");

		} if(dry_calib) {
			ESP_LOGI(TAG, "EC Dry Calibration Started");
            calibrate_sensor(&ec_sensor, &calibrate_ec_dry, &dev);
            dry_calib = false;
            if (!get_is_grow_active()) {
                ESP_LOGI(TAG, "EC task Suspended");
                vTaskSuspend(*sensor_get_task_handle(&ec_sensor));
            }
            ESP_LOGI(TAG, "EC Dry Calibration Completed");

		} else {		// EC sensor is Active
			read_ec_with_temperature(&dev, sensor_get_value(get_water_temp_sensor()), sensor_get_address_value(&ec_sensor));
			ESP_LOGI(TAG, "EC: %f", sensor_get_value(&ec_sensor));

			// Sync with other sensor tasks
			// Wait up to 10 seconds to let other tasks end
			xEventGroupSync(sensor_event_group, EC_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
		}
	}
}
