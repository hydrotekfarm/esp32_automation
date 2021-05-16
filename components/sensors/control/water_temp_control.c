#include "water_temp_control.h"

#include <esp_log.h>

#include "rf_transmitter.h"
#include "nvs_namespace_keys.h"
#include "sensor.h"
#include "water_temp_reading.h"

struct sensor_control* get_water_temp_control() { return &water_temp_control; }

void check_water_temp() {
    int result = control_check_sensor(&water_temp_control, sensor_get_value(get_water_temp_sensor()));
    if(!is_water_cooler_on && result == -1) {
        heat_water();
        is_water_cooler_on = true;
    } else if(!is_water_cooler_on && result == 1) {
        cool_water();
        is_water_cooler_on = true;
    } else if(is_water_cooler_on && result == 0) {
        stop_water_adjustment();
        is_water_cooler_on = false;
    }
}

void heat_water() {
    ESP_LOGI(WATER_TEMP_TAG, "Turning on water heater");
    control_power_outlet(WATER_HEATER, true);
}

void cool_water() {
    ESP_LOGI(WATER_TEMP_TAG, "Turning on water cooler");
    control_power_outlet(WATER_COOLER, true);
}

void stop_water_adjustment() {
    ESP_LOGI(WATER_TEMP_TAG, "Turning off water heater and cooler");
    control_power_outlet(WATER_HEATER, false);
    control_power_outlet(WATER_COOLER, false);
}

void water_temp_update_settings(cJSON *item) {
    nvs_handle_t *handle = nvs_get_handle(WATER_TEMP_NVS_NAMESPACE);
	control_update_settings(&water_temp_control, item, handle);

	nvs_commit_data(handle);
	ESP_LOGI(WATER_TEMP_TAG, "Updated settings and committed data to NVS");
}

void water_temp_get_nvs_settings() {
	control_get_nvs_settings(&water_temp_control, WATER_TEMP_NVS_NAMESPACE);
	ESP_LOGI(WATER_TEMP_TAG ,"Updated settings from NVS");
}