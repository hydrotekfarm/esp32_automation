idf_component_register(
	SRCS "control/control_task.c" "control/ec_control.c" "control/ph_control.c" "libs/ds18x20.c" "libs/ds3231.c" "libs/ec_sensor.c" "libs/i2cdev.c" "libs/onewire.c" "libs/ph_sensor.c" "libs/rf_transmission.c" "libs/ultrasonic.c" "reading/ec_reading.c" "reading/ph_reading.c" "reading/sync_sensors.c" "reading/ultrasonic_reading.c" "reading/water_temp_reading.c"
	INCLUDE_DIRS "control/" "libs/" "reading/" 	
	REQUIRES boot rtc nvs_flash
	PRIV_REQUIRES 
)