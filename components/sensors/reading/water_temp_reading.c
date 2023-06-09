#include "water_temp_reading.h"

#include <esp_err.h>
#include <esp_log.h>

#include "ds18x20.h"
#include "sync_sensors.h"
#include "ports.h"
#include "ph_reading.h"

struct sensor* get_water_temp_sensor() { return &water_temp_sensor; }

void measure_water_temperature(void *parameter) {		// Water Temperature Measurement Task
	const char *TAG = "Temperature_Task";

	init_sensor(&water_temp_sensor, "water_temp", true, false);

	ds18x20_addr_t ds18b20_address[1];

	gpio_config_t temperature_gpio_config = { (BIT(TEMPERATURE_SENSOR_GPIO)), GPIO_MODE_OUTPUT };
    gpio_config(&temperature_gpio_config);

	// Scan and setup sensor
	uint32_t sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO,
			ds18b20_address, 1);

	sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO,
			ds18b20_address, 1);
	vTaskDelay(pdMS_TO_TICKS(1000));

	if(sensor_count < 1) ESP_LOGE(TAG, "Sensor Not Found");

	for (;;) {
		// Perform Temperature Calculation and Read Temperature; vTaskDelay in the source code of this function
		esp_err_t error = ds18x20_measure_and_read(TEMPERATURE_SENSOR_GPIO,
				ds18b20_address[0], sensor_get_address_value(&water_temp_sensor));
		// Error Management
		if (error == ESP_OK) {
			ESP_LOGI(TAG, "temperature: %f\n", sensor_get_value(&water_temp_sensor));
		} else if (error == ESP_ERR_INVALID_RESPONSE) {
			ESP_LOGE(TAG, "Temperature Sensor Not Connected\n");
		} else if (error == ESP_ERR_INVALID_CRC) {
			ESP_LOGE(TAG, "Invalid CRC, Try Again\n");
		} else {
			ESP_LOGE(TAG, "Unknown Error\n");
		}

		// Sync with other sensor tasks
		// Wait up to 10 seconds to let other tasks end
		if (!sensor_calib_status(get_ph_sensor())) {
                xEventGroupSync(sensor_event_group, WATER_TEMPERATURE_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
        } else {
			//If ph calibration on, get frequent water temp readings// 
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
	}
}
