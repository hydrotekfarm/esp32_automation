#include "scd40_reading.h"
#include <esp_log.h>
#include <string.h>
#include "scd40.h"
#include "sync_sensors.h"
#include "task_priorities.h"
#include "ports.h"

struct sensor* get_temperature_sensor() { return &temperature_sensor; }

struct sensor* get_humidity_sensor() { return &humidity_sensor;}

struct sensor* get_co2_sensor() { return &co2_sensor;}

void measure_scd40(void *parameter) {     // SCD40 Sensor Measurement Task
    const char *TAG = "SCD40_Task";

    init_sensor(&temperature_sensor, "temperature", true, false);

    init_sensor(&humidity_sensor, "humidity", true, false);

    init_sensor(&co2_sensor, "humidity", true, false);

    scd40_sensor_t dev;
	memset(&dev, 0, sizeof(scd40_sensor_t));
	ESP_ERROR_CHECK(scd40_init_desc(&dev, 0, SCD40_I2C_ADDR, SDA_GPIO, SCL_GPIO)); // Initialize SCD40 I2C communication

    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_ERROR_CHECK(scd40_start_periodic_measurement(&dev)); 

    for(;;) {
        // trigger the sensor to start one TPHG measurement cycle
        bool check = false; 

        vTaskDelay(pdMS_TO_TICKS(1000));

        if (scd40_get_data_ready_status(&dev, &check) == ESP_OK) {
            // get the results and do something with them
            if (check) {
                float temp = 0.0f, humidity = 0.0f; 
                uint16_t co2 = 0;
                scd40_read_measurement(&dev, &co2, &temp, &humidity);
                ESP_LOGI(TAG, "CO2: %.2f", (float)co2);
                ESP_LOGI(TAG, "Temperature: %.2f", temp);
                ESP_LOGI(TAG, "Humidity: %.2f\n", humidity);

                float *co2_val = sensor_get_address_value(&co2_sensor);
                *co2_val = (float)co2;

                float *temp_val = sensor_get_address_value(&temperature_sensor);
                *temp_val = temp;
                
                 float *humidity_val = sensor_get_address_value(&humidity_sensor);
                *humidity_val = humidity; 

            } else {
                //ESP_LOGI(TAG, "Data not ready yet");
            }
        } else {
               // ESP_LOGE(TAG, "Not OK!!");
        }
        // Set SCD BIT
        xEventGroupSetBits(sensor_event_group, SCD_BIT);
    }

}
