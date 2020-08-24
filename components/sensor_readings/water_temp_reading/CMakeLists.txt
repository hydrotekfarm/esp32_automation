idf_component_register(
	SRCS "water_temp_reading.c"
	INCLUDE_DIRS "." 	
	PRIV_REQUIRES ds18x20 sync_sensors port_manager task_manager
)