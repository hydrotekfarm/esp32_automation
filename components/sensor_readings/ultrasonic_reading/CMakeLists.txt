idf_component_register(
	SRCS "ultrasonic_reading.c"
	INCLUDE_DIRS "." 	
	PRIV_REQUIRES ultrasonic sync_sensors port_manager task_manager
)