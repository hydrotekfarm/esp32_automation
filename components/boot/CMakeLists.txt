idf_component_register(
	SRCS "boot.c"
	INCLUDE_DIRS "." 	
	PRIV_REQUIRES nvs_flash port_manager sensor_readings rtc mqtt_manager sensor_control
)