idf_component_register(
	SRCS 
	"control/co2_control.c"
	"control/control_task.c" 
	"control/ec_control.c" 
	"control/ph_control.c" 
	"control/water_temp_control.c"
	"control/reservoir_control.c" 
	"control/sensor_control.c"
	"libs/co2_sensor.c"
	"libs/ds18x20.c" 
	"libs/ec_sensor.c" 
	"libs/i2cdev.c" 
	"libs/mcp23x17.c" 
	"libs/onewire.c" 
	"libs/ph_sensor.c" 
	"reading/co2_reading.c"
	"reading/ec_reading.c" 
	"reading/ph_reading.c" 
	"reading/sensor.c"
	"reading/sync_sensors.c" 
	"reading/water_temp_reading.c"
	INCLUDE_DIRS "control/" "libs/" "reading/" 	
	REQUIRES boot rtc rf_transmitter nvs_flash json log nvs_manager nvs_flash network_manager
	PRIV_REQUIRES 
)