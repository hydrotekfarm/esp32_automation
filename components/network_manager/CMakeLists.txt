idf_component_register(
	SRCS "access_point/access_point.c" "mqtt/mqtt_manager.c" "wifi/wifi_connect.c"
	INCLUDE_DIRS "access_point/" "mqtt/" "wifi/" 	
	PRIV_REQUIRES mqtt boot sensors rtc json nvs_manager
	REQUIRES esp_http_server
)

