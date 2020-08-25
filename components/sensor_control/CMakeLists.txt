idf_component_register(
	SRCS "control_task.c" "ec_control.c" "ph_control.c"
	INCLUDE_DIRS "." 	
	REQUIRES boot rtc sensor_readings
	PRIV_REQUIRES 
)