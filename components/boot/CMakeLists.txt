idf_component_register(
	SRCS "boot.c" "app_connect.c" "ports.c"
	INCLUDE_DIRS "." 	
	REQUIRES esp_adc_cal sensors
	PRIV_REQUIRES nvs_flash sensors rtc mqtt_manager rf_transmitter esp_http_server nvs_manager
)