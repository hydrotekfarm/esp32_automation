set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES log nvs_flash esp_adc_cal)
set(COMPONENT_ADD_INCLUDEDIRS 
	"../../main/"
	"../port_manager/"
	"../task_manager/"
)

register_component()
