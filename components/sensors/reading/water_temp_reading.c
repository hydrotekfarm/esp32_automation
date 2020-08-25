#include "water_temp_reading.h"

#include <esp_err.h>
#include <esp_log.h>

#include "ds18x20.h"
#include "sync_sensors.h"
#include "ports.h"

void measure_water_temperature(void *parameter) {		// Water Temperature Measurement Task
	const char *TAG = "Temperature_Task";

	_water_temp = 0;

	ds18x20_addr_t ds18b20_address[1];

	// Scan and setup sensor
	uint32_t sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO,
			ds18b20_address, 1);

	sensor_count = ds18x20_scan_devices(TEMPERATURE_SENSOR_GPIO,
			ds18b20_address, 1);
	vTaskDelay(pdMS_TO_TICKS(1000));

	if(sensor_count < 1 && water_temperature_active) ESP_LOGE(TAG, "Sensor Not Found");

	for (;;) {
		// Perform Temperature Calculation and Read Temperature; vTaskDelay in the source code of this function
		esp_err_t error = ds18x20_measure_and_read(TEMPERATURE_SENSOR_GPIO,
				ds18b20_address[0], &_water_temp);
		// Error Management
		if (error == ESP_OK) {
			ESP_LOGI(TAG, "temperature: %f\n", _water_temp);
		} else if (error == ESP_ERR_INVALID_RESPONSE) {
			ESP_LOGE(TAG, "Temperature Sensor Not Connected\n");
		} else if (error == ESP_ERR_INVALID_CRC) {
			ESP_LOGE(TAG, "Invalid CRC, Try Again\n");
		} else {
			ESP_LOGE(TAG, "Unknown Error\n");
		}

		// Sync with other sensor tasks
		// Wait up to 10 seconds to let other tasks end
		xEventGroupSync(sensor_event_group, WATER_TEMPERATURE_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}
