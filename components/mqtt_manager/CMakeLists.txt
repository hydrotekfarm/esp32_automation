set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES log nvs_flash mqtt)
set(COMPONENT_ADD_INCLUDEDIRS
	"../task_manager/"
	"../boot/"
)

register_component()
