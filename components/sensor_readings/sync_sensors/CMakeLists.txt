idf_component_register(
	SRCS "sync_sensors.c"
	INCLUDE_DIRS "." 	
	PRIV_REQUIRES ec_reading ph_reading ultrasonic_reading water_temp_reading
)