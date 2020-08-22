#include "task_manager.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include "mqtt_manager.h"
#include "ec_reading.h"
#include "ph_reading.h"
#include "ultrasonic_reading.h"
#include "water_temp_reading.h"
#include "sync_sensors.h"
#include "rtc.h"

void create_tasks() {
	// Create core 0 tasks
	xTaskCreatePinnedToCore(manage_timers_alarms, "timer_alarm_task", 2500, NULL, TIMER_ALARM_TASK_PRIORITY, &timer_alarm_task_handle, 0);
	xTaskCreatePinnedToCore(publish_data, "publish_task", 2500, NULL, MQTT_PUBLISH_TASK_PRIORITY, &publish_task_handle, 0);
	xTaskCreatePinnedToCore(sensor_control, "sensor_control_task", 2500, NULL, SENSOR_CONTROL_TASK_PRIORITY, &sensor_control_task_handle, 0);

	// Create core 1 tasks
	xTaskCreatePinnedToCore(measure_water_temperature, "temperature_task", 2500, NULL, WATER_TEMPERATURE_TASK_PRIORITY, &water_temperature_task_handle, 1);
	xTaskCreatePinnedToCore(measure_ec, "ec_task", 2500, NULL, EC_TASK_PRIORITY, &ec_task_handle, 1);
	xTaskCreatePinnedToCore(measure_ph, "ph_task", 2500, NULL, PH_TASK_PRIORITY, &ph_task_handle, 1);
	xTaskCreatePinnedToCore(measure_distance, "ultrasonic_task", 2500, NULL, ULTRASONIC_TASK_PRIORITY, &ultrasonic_task_handle, 1);
	xTaskCreatePinnedToCore(sync_task, "sync_task", 2500, NULL, SYNC_TASK_PRIORITY, &sync_task_handle, 1);
}

void delay_task(uint32_t duration) {
	vTaskDelay(pdMS_TO_TICKS(duration));
}

void set_priority(TaskHandle_t task_handle, uint32_t priority) {
	vTaskPrioritySet(task_handle, priority);
}

void set_max_priority(TaskHandle_t task_handle) {
	vTaskPrioritySet(task_handle, configMAX_PRIORITIES - 1);
}


