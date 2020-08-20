# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES mqtt nvs_flash esp_adc_cal)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS 
	"../components/sensor_libs/ds18x20"
	"../components/sensor_libs/ds3231"
	"../components/sensor_libs/ec_sensor"
	"../components/sensor_libs/esp_idf_lib_helpers"
	"../components/sensor_libs/i2cdev"
	"../components/sensor_libs/onewire"
	"../components/sensor_libs/ph_sensor"
	"../components/sensor_libs/rf_transmission"
	"../components/sensor_libs/ultrasonic"
	"../components/boot/"
)

register_component()