idf_component_register(
	SRCS "ph_reading.c"
	INCLUDE_DIRS "." 	
	PRIV_REQUIRES ph_sensor sync_sensors port_manager task_manager
)