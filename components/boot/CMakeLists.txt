idf_component_register(
	SRCS "boot.c" "ports.c"
	INCLUDE_DIRS "." 	
	REQUIRES esp_adc_cal sensors
	PRIV_REQUIRES nvs_flash rtc mqtt_manager rf_transmitter
)