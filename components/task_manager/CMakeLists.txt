set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES log freertos)
set(COMPONENT_ADD_INCLUDEDIRS 
	"../mqtt_manager"
	"../sensor_readings/ec_reading/"
	"../sensor_readings/ph_reading/"
	"../sensor_readings/ultrasonic_reading/"
	"../sensor_readings/water_temp_reading/"
	"../sensor_readings/sync_sensors/"
	"../rtc/"
)

register_component()
