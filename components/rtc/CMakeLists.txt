idf_component_register(
	SRCS "rtc.c"
	INCLUDE_DIRS "." 	
	REQUIRES sensor_libs
	PRIV_REQUIRES port_manager sensor_control boot
)