#include <freertos/FreeRTOS.h>
#include <Freertos/task.h>
#include <freertos/event_groups.h>

#define SENSOR_MEASUREMENT_PERIOD 10000 // Measuring increment time in ms

// Task handle
TaskHandle_t sync_task_handle;

// Sensor Task Coordination with Event Group
EventGroupHandle_t sensor_event_group;
#define DELAY_BIT		    (1<<0)
#define WATER_TEMPERATURE_BIT	(1<<1)
#define EC_BIT 	        (1<<2)
#define PH_BIT		    (1<<3)
#define ULTRASONIC_BIT    (1<<4)

// Sensor sync bits
uint32_t sensor_sync_bits;

// Set sync bit based on active sensors
void set_sensor_sync_bits();

// Sync sensors together
void sync_task();
