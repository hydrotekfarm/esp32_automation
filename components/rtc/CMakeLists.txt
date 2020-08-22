set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES log nvs_flash )
set(COMPONENT_ADD_INCLUDEDIRS 
	"../sensor_libs/ds3231/"
	"../port_manager"
	"../task_manager"
	"../rtc/"
)

register_component()
