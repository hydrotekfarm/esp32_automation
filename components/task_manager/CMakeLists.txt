idf_component_register(
	SRCS "task_manager.c"
	INCLUDE_DIRS "." 	
	PRIV_REQUIRES mqtt_manager sensor_readings rtc sensor_control
)

