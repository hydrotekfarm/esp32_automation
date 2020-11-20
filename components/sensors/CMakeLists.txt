idf_component_register(
	SRCS 
	"control/control_task.c" 
	"control/reservoir_control.c" 
	"control/ec_control.c" 
	"control/ph_control.c" 
	"libs/ds18x20.c" 
	"libs/ds3231.c" 
	"libs/ec_sensor.c" 
	"libs/i2cdev.c" 
	"libs/mcp23x17.c" 
	"libs/onewire.c" 
	"libs/ph_sensor.c" 
	"reading/ec_reading.c" 
	"reading/ph_reading.c" 
	"reading/sync_sensors.c" 
	"reading/water_temp_reading.c"
	INCLUDE_DIRS "control/" "libs/" "reading/" 	
	REQUIRES boot rtc rf_transmitter nvs_flash json log
	PRIV_REQUIRES 
)