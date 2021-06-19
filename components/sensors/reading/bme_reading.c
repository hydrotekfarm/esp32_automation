#include "bme_reading.h"
#include <esp_log.h>
#include <string.h>
#include "bme680.h"
#include "sync_sensors.h"
#include "task_priorities.h"
#include "ports.h"

struct sensor* get_temperature_sensor() { return &temperature_sensor; }

struct sensor* get_humidity_sensor() { return &humidity_sensor;}

void measure_bme(void *parameter) {     // bme Sensor Measurement Task
    const char *TAG = "BME_Task";

    init_sensor(&temperature_sensor, "temperature", true, false);

    init_sensor(&humidity_sensor, "humidity", true, false);

    bme680_t sensor;
    memset(&sensor, 0, sizeof(bme680_t));

    ESP_ERROR_CHECK(bme680_init_desc(&sensor, BME680_I2C_ADDR_1, 0, SDA_GPIO, SCL_GPIO));

    // Init the sensor
    ESP_ERROR_CHECK(bme680_init_sensor(&sensor));

    // Changes the oversampling rates to 4x oversampling for temperature
    // and 2x oversampling for humidity. Pressure measurement is skipped.
    bme680_set_oversampling_rates(&sensor, BME680_OSR_4X, BME680_OSR_NONE, BME680_OSR_2X);

    // Change the IIR filter size for temperature and pressure to 7.
    bme680_set_filter_size(&sensor, BME680_IIR_SIZE_7);

    // Set ambient temperature
    bme680_set_ambient_temperature(&sensor, 22);

    // As long as sensor configuration isn't changed, duration is constant
    uint32_t duration;
    bme680_get_measurement_duration(&sensor, &duration);

    bme680_values_float_t values; 
    for(;;) {
        // trigger the sensor to start one TPHG measurement cycle
        if (bme680_force_measurement(&sensor) == ESP_OK) {
            // passive waiting until measurement results are available
            vTaskDelay(duration);
            // get the results and do something with them
            if (bme680_get_results_float(&sensor, &values) == ESP_OK) {
                ESP_LOGI(TAG, "Temperature: %.2f", values.temperature);
                ESP_LOGI(TAG, "Humidity: %.2f", values.humidity);

                float *temp = sensor_get_address_value(&temperature_sensor);
                *temp = values.temperature;
                
                float *humidity = sensor_get_address_value(&humidity_sensor);
                *humidity = values.humidity; 

            } else {
                ESP_LOGE(TAG, "Unable to get valid readings for temperature and humidity");
            }
        }

        // Sync with other sensor tasks
        // Wait up to 10 seconds to let other tasks end
        xEventGroupSync(sensor_event_group, BME_BIT, sensor_sync_bits, pdMS_TO_TICKS(SENSOR_MEASUREMENT_PERIOD));
    }

}
