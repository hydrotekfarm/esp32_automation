set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES log nvs_flash freertos)
set(COMPONENT_ADD_INCLUDEDIRS
	"../ec_reading/"
	"../ph_reading/"
	"../ultrasonic_reading/"
	"../water_temp_reading/"
)

register_component()
