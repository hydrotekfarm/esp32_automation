// Core 0 Task Priorities
#define TIMER_ALARM_TASK_PRIORITY 0
#define MQTT_PUBLISH_TASK_PRIORITY 1
#define SENSOR_CONTROL_TASK_PRIORITY 2
#define RF_TRANSMITTER_TASK_PRIORITY 3 // RF Transmitter should be higher than other priorities

// Core 1 Task Priorities
#define ULTRASONIC_TASK_PRIORITY 0
#define PH_TASK_PRIORITY 1
#define EC_TASK_PRIORITY 2
#define WATER_TEMPERATURE_TASK_PRIORITY 3
#define SYNC_TASK_PRIORITY 4
