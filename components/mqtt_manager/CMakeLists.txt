idf_component_register(
	SRCS "mqtt_manager.c"
	INCLUDE_DIRS "." 	
	PRIV_REQUIRES mqtt task_manager boot sensor_readings rtc
)

