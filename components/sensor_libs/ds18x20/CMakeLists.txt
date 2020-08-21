set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES onewire freertos log esp_idf_lib_helpers)

set(COMPONENT_ADD_INCLUDEDIRS
	"../onewire/"
)

register_component()
