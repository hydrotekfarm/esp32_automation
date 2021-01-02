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

	init_sensor(&ec_sensor, "ec", true, false);
	dry_calib = false;

	ec_sensor_t dev;
	memset(&dev, 0, sizeof(ec_sensor_t));
	ESP_ERROR_CHECK(ec_init(&dev, 0, EC_ADDR_BASE, SDA_GPIO, SCL_GPIO)); // Initialize EC I2C communication

	for (;;) {
		if(sensor_calib_status(&ec_sensor)) { // Calibration Mode is activated
			calibrate_sensor(&ec_sensor, &calibrate_ec, &dev);
			sensor_set_calib_status(&ec_sensor, false);
		} if(dry_calib) {
			calibrate_sensor(&ec_sensor, &calibrate_ec_dry, &dev);
			dry_calib = false;
		}	else {		// EC sensor is Active
			read_ec_with_temperature(&dev, sensor_get_value(get_water_temp_sensor()), sensor_get_address_value(&ec_sensor));
			ESP_LOGI(TAG, "EC: %f", sensor_get_value(&ec_sensor));

			// Sync with other sensor tasks
			// Wait up to 10 seconds to let other tasks end
			xEventGroupSync(sensor_event_group, EC_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
		}
	}
}
