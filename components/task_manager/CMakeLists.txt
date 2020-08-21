set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES log freertos)
set(COMPONENT_ADD_INCLUDEDIRS 
	"../mqtt_manager"
)

register_component()
