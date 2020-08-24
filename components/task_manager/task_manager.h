#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Core 0 Task Priorities
#define TIMER_ALARM_TASK_PRIORITY 0
#define MQTT_PUBLISH_TASK_PRIORITY 1
#define SENSOR_CONTROL_TASK_PRIORITY 2

// Core 1 Task Priorities
#define ULTRASONIC_TASK_PRIORITY 0
#define PH_TASK_PRIORITY 1
#define EC_TASK_PRIORITY 2
#define WATER_TEMPERATURE_TASK_PRIORITY 3
#define SYNC_TASK_PRIORITY 4

#define SENSOR_MEASUREMENT_PERIOD 10000 // Measuring increment time in ms

// Task Handles
TaskHandle_t water_temperature_task_handle;
TaskHandle_t ec_task_handle;
TaskHandle_t ph_task_handle;
TaskHandle_t ultrasonic_task_handle;
TaskHandle_t sync_task_handle;
TaskHandle_t publish_task_handle;
TaskHandle_t timer_alarm_task_handle;
TaskHandle_t sensor_control_task_handle;

// Create tasks
void create_tasks();

// Delay task
void delay_task(uint32_t duration);

// Set task to given priority
void set_priority(TaskHandle_t task_handle, uint32_t priority);

// Set task to max priority
void set_max_priority(TaskHandle_t priority);
