idf_component_register(
	SRCS "task_manager.c"
	INCLUDE_DIRS "." 	
	PRIV_REQUIRES mqtt_manager ec_reading ph_reading ultrasonic_reading water_temp_reading sync_sensors rtc
)

