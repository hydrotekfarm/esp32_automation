#include "scd30_reading.h"
#include <esp_log.h>
#include <string.h>
#include "scd30.h"
#include "sync_sensors.h"
#include "task_priorities.h"
#include "ports.h"

struct sensor* get_temperature_sensor() { return &temperature_sensor; }

struct sensor* get_humidity_sensor() { return &humidity_sensor;}

struct sensor* get_co2_sensor() { return &co2_sensor;}

void measure_scd30(void *parameter) {     // SCD30 Sensor Measurement Task
    const char *TAG = "SCD30_Task";

    init_sensor(&temperature_sensor, "temperature", true, false);

    init_sensor(&humidity_sensor, "humidity", true, false);

    init_sensor(&co2_sensor, "humidity", true, false);

    scd30_sensor_t dev;
	memset(&dev, 0, sizeof(scd30_sensor_t));
	ESP_ERROR_CHECK(scd30_init(&dev, 0, SCD30_I2C_ADDR, SDA_GPIO, SCL_GPIO)); // Initialize SCD30 I2C communication
    vTaskDelay(pdMS_TO_TICKS(10000));

    ESP_ERROR_CHECK(scd30_trigger_continuous_measurement(&dev, 0));
    for(;;) {
        // trigger the sensor to start one TPHG measurement cycle
        bool check = false; 
        unsigned int tick = xTaskGetTickCount();
        while (!check && (xTaskGetTickCount() - tick < 500)) {
            ESP_ERROR_CHECK(scd30_get_data_ready_status(&dev, &check)); 
        }
        if (check) {
            // get the results and do something with them
            float temp = 0.0f, co2 = 0.0f, humidity = 0.0f; 
            scd30_read_measurement(&dev, &co2, &temp, &humidity);
            ESP_LOGI(TAG, "CO2: %.2f", co2);
            ESP_LOGI(TAG, "Temperature: %.2f", temp);
            ESP_LOGI(TAG, "Humidity: %.2f", humidity);

            float *co2_val = sensor_get_address_value(&co2_sensor);
            *co2_val = temp;

            float *temp_val = sensor_get_address_value(&temperature_sensor);
            *temp_val = temp;
                
            float *humidity_val = sensor_get_address_value(&humidity_sensor);
            *humidity_val = humidity; 
        } else {
                ESP_LOGE(TAG, "Unable to get readings for co2, temperature, humidity in time for this cycle.");
        }

        // Sync with other sensor tasks
        // Wait up to 10 seconds to let other tasks end
        xEventGroupSync(sensor_event_group, SCD_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
    }

}
