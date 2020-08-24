idf_component_register(
	SRCS "mqtt_manager.c"
	INCLUDE_DIRS "." 	
	PRIV_REQUIRES mqtt task_manager boot ec_reading ph_reading ultrasonic_reading water_temp_reading rtc
)

