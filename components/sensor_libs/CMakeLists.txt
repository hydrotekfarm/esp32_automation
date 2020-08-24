idf_component_register(
	SRCS "ds18x20.c" "ds3231.c" "ec_sensor.c" "i2cdev.c" "onewire.c" "ph_sensor.c" "rf_transmission.c" "ultrasonic.c"
	INCLUDE_DIRS "." 	
	REQUIRES nvs_flash
	PRIV_REQUIRES 
)