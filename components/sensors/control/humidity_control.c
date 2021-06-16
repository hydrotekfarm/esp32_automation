#include "humidity_control.h"

#include <esp_log.h>

#include "rf_transmitter.h"
#include "nvs_namespace_keys.h"
#include "sensor.h"
#include "bme_reading.h"

struct sensor_control* get_humidity_control() { return &humidity_control; }

void check_humidity() {
    int result = control_check_sensor(&humidity_control, sensor_get_value(get_humidity_sensor()));
    if(!is_humidifier_on && result == -1) {
        increase_humidity();
        is_humidifier_on = true;
    } else if(!is_humidifier_on && result == 1) {
        decrease_humidity();
        is_humidifier_on = true;
    } else if(is_humidifier_on && result == 0) {
        stop_humidity_adjustment();
        is_humidifier_on = false;
    }
}

void increase_humidity() {
    ESP_LOGI(HUMIDITY_TAG, "Increasing Humidity");
    control_power_outlet(HUMIDIFIER_INCREASE, true);
}

void decrease_humidity() {
    ESP_LOGI(HUMIDITY_TAG, "Decreasing Humidity");
    control_power_outlet(HUMIDIFIER_DECREASE, true);
}

void stop_humidity_adjustment() {
    ESP_LOGI(HUMIDITY_TAG, "Turning off humidifier control");
    control_power_outlet(HUMIDIFIER_INCREASE, false);
    control_power_outlet(HUMIDIFIER_DECREASE, false);
}

void humidity_update_settings(cJSON *item) {
    nvs_handle_t *handle = nvs_get_handle(HUMIDITY_NVS_NAMESPACE);
	control_update_settings(&humidity_control, item, handle);

	nvs_commit_data(handle);
	ESP_LOGI(HUMIDITY_TAG, "Updated settings and committed data to NVS");
}

void humidity_get_nvs_settings() {
	control_get_nvs_settings(&humidity_control, HUMIDITY_NVS_NAMESPACE);
	ESP_LOGI(HUMIDITY_TAG ,"Updated settings from NVS");
}