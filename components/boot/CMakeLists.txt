idf_component_register(
	SRCS "boot.c" "access_point_mode.c"
	INCLUDE_DIRS "." 	
	REQUIRES esp_adc_cal
	PRIV_REQUIRES nvs_flash sensors rtc mqtt_manager rf_transmitter esp_http_server
)