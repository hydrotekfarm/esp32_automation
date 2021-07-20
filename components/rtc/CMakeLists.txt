idf_component_register(
	SRCS "ds3231.c" "rtc.c"
	INCLUDE_DIRS "." 	
	REQUIRES sensors
	PRIV_REQUIRES boot grow_manager
)