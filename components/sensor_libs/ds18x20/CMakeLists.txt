idf_component_register(
	SRCS "ds18x20.c"
	INCLUDE_DIRS "." 	
	REQUIRES onewire
	PRIV_REQUIRES esp_idf_lib_helpers
)