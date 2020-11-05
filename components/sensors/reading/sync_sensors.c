#include "sync_sensors.h"

#include <esp_log.h>

#include "ec_reading.h"
#include "ph_reading.h"
#include "ultrasonic_reading.h"
#include "water_temp_reading.h"

void set_sensor_sync_bits() {
	sensor_sync_bits = DELAY_BIT | EC_BIT | PH_BIT | (sensor_get_active_status(get_water_temp_sensor()) ? WATER_TEMPERATURE_BIT : 0);
}

void sync_task(void *parameter) {				// Sensor Synchronization Task
	const char *TAG = "Sync_Task";
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
