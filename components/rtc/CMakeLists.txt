idf_component_register(
	SRCS "rtc.c"
	INCLUDE_DIRS "." 	
	REQUIRES ds3231
	PRIV_REQUIRES port_manager task_manager
)