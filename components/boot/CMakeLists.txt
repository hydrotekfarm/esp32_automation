idf_component_register(
	SRCS "boot.c"
	INCLUDE_DIRS "." 	
	REQUIRES esp_adc_cal
	PRIV_REQUIRES nvs_flash sensors rtc mqtt_manager 
)