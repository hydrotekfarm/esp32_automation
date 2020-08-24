idf_component_register(
	SRCS "ds3231.c"
	INCLUDE_DIRS "." 	
	REQUIRES i2cdev
	PRIV_REQUIRES esp_idf_lib_helpers
)
