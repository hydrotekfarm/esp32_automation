set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES log nvs_flash)
set(COMPONENT_ADD_INCLUDEDIRS
	"../../sensor_libs/ds18x20"
	"../sync_sensors"
	"../../port_manager/"
	"../../task_manager/"
)

register_component()
