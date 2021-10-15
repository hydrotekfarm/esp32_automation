idf_component_register(
	SRCS "rtc.c"
	INCLUDE_DIRS "." 	
	REQUIRES sensors shared_libs
	PRIV_REQUIRES boot grow_manager
)