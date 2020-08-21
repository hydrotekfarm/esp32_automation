set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES log nvs_flash esp_adc_cal freertos port_manager task_manager)
set(COMPONENT_ADD_INCLUDEDIRS 
	"../port_manager"
	"../task_manager"
	"../sensor_readings/sync_sensors/"
)

register_component()
