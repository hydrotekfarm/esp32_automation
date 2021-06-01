#include "ph_reading.h"
#include <esp_log.h>
#include <string.h>
#include "co2_sensor.h"
#include "sync_sensors.h"
#include "task_priorities.h"
#include "ports.h"

struct sensor* get_co2_sensor() { return &co2_sensor; }

void measure_co2(void *parameter) {		// co2 Sensor Measurement Task
	const char *TAG = "CO2_Task";

	init_sensor(&co2_sensor, "co2", true, false);

	co2_sensor_t dev;
	memset(&dev, 0, sizeof(co2_sensor_t));

	ESP_ERROR_CHECK(co2_init(&dev, 0, CO2_ADDR_BASE, SDA_GPIO, SCL_GPIO)); // Initialize CO2 I2C communication
    vTaskDelay(pdMS_TO_TICKS(10000));
	for (;;) {
			read_co2(&dev, sensor_get_address_value(&co2_sensor));
			ESP_LOGI(TAG, "co2: %i", sensor_get_value(&co2_sensor));

			// Sync with other sensor tasks and wait up to 10 seconds to let other tasks end
            /*
			xEventGroupSync(sensor_event_group, PH_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
            */
	}
}