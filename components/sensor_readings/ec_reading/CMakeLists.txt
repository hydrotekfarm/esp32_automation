idf_component_register(
	SRCS "ec_reading.c"
	INCLUDE_DIRS "." 	
	PRIV_REQUIRES ec_sensor sync_sensors port_manager task_manager water_temp_reading
)