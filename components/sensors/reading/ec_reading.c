#include "ec_reading.h"
#include "grow_manager.h"
#include <esp_log.h>
#include "string.h"
#include "sync_sensors.h"
#include "task_priorities.h"
#include "ports.h"
#include "water_temp_reading.h"
#include <stdbool.h>

struct sensor *get_ec_sensor() { return &ec_sensor; }

ec_sensor_t *get_ec_dev() { return &ec_dev; }

bool get_is_ec_activated() { return is_ec_activated; }

void set_is_ec_activated(bool is_active) { is_ec_activated = is_active; }

void measure_ec(void *parameter) {				// EC Sensor Measurement Task
	const char *TAG = "EC_Task";

	init_sensor(&ec_sensor, "ec", true, false);
	dry_calib = false;

	memset(&ec_dev, 0, sizeof(ec_sensor_t));

	if(ec_init(&ec_dev, 0, EC_ADDR_BASE, SDA_GPIO, SCL_GPIO)==ESP_OK) {  // Initialize EC I2C communication
		ESP_LOGI(TAG, "EC Sensor Initialized");
	}
	else {
		ESP_LOGE(TAG, "EC Sensor Initialization Failed");
	}

	is_ec_activated = false;
    
	

	while(!get_is_ec_activated() ) {
		if(activate_ec(&ec_dev)==ESP_OK) {
			is_ec_activated = true;
			ESP_LOGI(TAG, "EC activated");
			break;
      }
		ESP_LOGE(TAG, "EC not activated");
		vTaskDelay(pdMS_TO_TICKS(10000));
	
	}
	

   vTaskDelay(pdMS_TO_TICKS(10000));
	for (;;) {
		if(sensor_calib_status(&ec_sensor)) { // Calibration Mode is activated
			ESP_LOGI(TAG, "EC Wet Calibration Started");
            calibrate_sensor(&ec_sensor, &calibrate_ec, &ec_dev);
            sensor_set_calib_status(&ec_sensor, false);
            if (!get_is_grow_active()) {
                ESP_LOGI(TAG, "EC task Suspended");
                vTaskSuspend(*sensor_get_task_handle(&ec_sensor));
            }
            ESP_LOGI(TAG, "EC Wet Calibration Completed");

		} if(dry_calib) {
			ESP_LOGI(TAG, "EC Dry Calibration Started");
            calibrate_sensor(&ec_sensor, &calibrate_ec_dry, &ec_dev);
            dry_calib = false;
            if (!get_is_grow_active()) {
                ESP_LOGI(TAG, "EC task Suspended");
                vTaskSuspend(*sensor_get_task_handle(&ec_sensor));
            }
            ESP_LOGI(TAG, "EC Dry Calibration Completed");

		} else {		// EC sensor is Active
			if (!get_is_ec_activated()) {
				 while(!get_is_ec_activated() ) {
					if(activate_ec(&ec_dev)==ESP_OK) {
						is_ec_activated = true;
						ESP_LOGI(TAG, "EC activated");
						break;
					}
						ESP_LOGE(TAG, "EC not activated");
						vTaskDelay(pdMS_TO_TICKS(10000));
	
	       		}
			}
			read_ec_with_temperature(&ec_dev, sensor_get_value(get_water_temp_sensor()), sensor_get_address_value(&ec_sensor));
			ESP_LOGI(TAG, "EC: %f", sensor_get_value(&ec_sensor));

			// Sync with other sensor tasks
			// Wait up to 10 seconds to let other tasks end
			xEventGroupSync(sensor_event_group, EC_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
		}
	}
}