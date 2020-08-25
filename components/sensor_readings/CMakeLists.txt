idf_component_register(
	SRCS "ec_reading.c" "ph_reading.c" "sync_sensors.c" "ultrasonic_reading.c" "water_temp_reading.c"
	INCLUDE_DIRS "." 	
	PRIV_REQUIRES sensor_libs boot
)