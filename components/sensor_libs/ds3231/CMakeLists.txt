set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES i2cdev log esp_idf_lib_helpers)

set(COMPONENT_ADD_INCLUDEDIRS 
	"../i2cdev/"
)

register_component()
