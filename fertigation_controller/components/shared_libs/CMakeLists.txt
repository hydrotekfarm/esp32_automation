idf_component_register(
	SRCS "ds3231.c"
	INCLUDE_DIRS "." 	
	REQUIRES sensors
	PRIV_REQUIRES
)