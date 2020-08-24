#include "sync_sensors.h"

#include <esp_log.h>

#include "ec_reading.h"
#include "ph_reading.h"
#include "ultrasonic_reading.h"
#include "water_temp_reading.h"

void set_sensor_sync_bits() {
	sensor_sync_bits = DELAY_BIT | (water_temperature_active ? WATER_TEMPERATURE_BIT : 0) | (ec_active ? EC_BIT : 0) | (ph_active ? PH_BIT : 0) | (ultrasonic_active ? ULTRASONIC_BIT : 0);
}

void sync_task(void *parameter) {				// Sensor Synchronization Task
	const char *TAG = "Sync_Task";
	sensor_sync_bits = 0;
	EventBits_t returnBits;
	for (;;) {
		// Provide a minimum amount of time before event group round resets
		vTaskDelay(pdMS_TO_TICKS(10000));
		returnBits = xEventGroupSync(sensor_event_group, DELAY_BIT, sensor_sync_bits, pdMS_TO_TICKS(10000));
		// Check Whether all tasks were completed on time
		if ((returnBits & sensor_sync_bits) == sensor_sync_bits) {
			ESP_LOGI(TAG, "Completed");
		} else {
			ESP_LOGE(TAG, "Failed to Complete On Time");
		}
	}
}
