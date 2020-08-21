set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES log nvs_flash mqtt)
set(COMPONENT_ADD_INCLUDEDIRS
	"../task_manager/"
	"../boot/"
	"../sensor_readings/ec_reading/"
	"../sensor_readings/ph_reading/"
	"../sensor_readings/ultrasonic_reading/"
	"../sensor_readings/water_temp_reading/"
)

register_component()
