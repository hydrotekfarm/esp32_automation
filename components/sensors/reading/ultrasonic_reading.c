#include "ultrasonic_reading.h"

#include <esp_log.h>
#include <esp_err.h>

#include "ultrasonic.h"
#include "sync_sensors.h"
#include "ports.h"

void measure_distance(void *parameter) {		// Ultrasonic Sensor Distance Measurement Task
	const char *TAG = "ULTRASONIC_TASK";

	_distance = 0;

	// Setting sensor ports and initializing
	ultrasonic_sensor_t sensor = { .trigger_pin = ULTRASONIC_TRIGGER_GPIO,
			.echo_pin = ULTRASONIC_ECHO_GPIO };

	ultrasonic_init(&sensor);

	for (;;) {
		// Measures distance and returns error code
		float distance;
		esp_err_t res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);

		// TODO check if value is beyond acceptable margin of error and react appropriately

		// React appropriately to error code
		switch (res) {
		case ESP_ERR_ULTRASONIC_PING:
			ESP_LOGE(TAG, "Device is in invalid state");
			break;
		case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
			ESP_LOGE(TAG, "Device not found");
			break;
		case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
			ESP_LOGE(TAG, "Distance is too large");
			break;
		default:
			ESP_LOGI(TAG, "Distance: %f cm\n", distance);
			_distance = distance;
		}

		// Sync with other sensor tasks
		// Wait up to 10 seconds to let other tasks end
		xEventGroupSync(sensor_event_group, ULTRASONIC_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
	}
}
