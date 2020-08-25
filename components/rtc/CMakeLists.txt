idf_component_register(
	SRCS "rtc.c"
	INCLUDE_DIRS "." 	
	REQUIRES sensor_libs
	PRIV_REQUIRES sensor_control boot
)