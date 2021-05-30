/*
 * ec_sensor.h
 *
 *  Created on: May 1, 2020
 *      Author: ajayv
 */

#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include <esp_err.h>
#include "i2cdev.h"

#define PH_ADDR_BASE 0x63

#ifdef __cplusplus
extern "C" {
#endif

typedef i2c_dev_t co2_sensor_t;

/**
 * @brief Setup co2 I2C communication
 * @param dev I2C device descriptor
 * @param port I2C port
 * @param addr I2C address
 * @param sda_gpio SDA GPIO
 * @param scl_gpio SCL GPIO
 * @return ESP_OK to indicate success
 */
esp_err_t co2_init(co2_sensor_t *dev, i2c_port_t port, uint8_t addr, int8_t sda_gpio, int8_t scl_gpio);

/**
 * @brief Calibrate pH sensor
 * @param dev I2C device descriptor
 * @param temperature This value is required for temperature compensation
 * @return ESP_OK to indicate success
 */
esp_err_t calibrate_ph(co2_sensor_t *dev, float temperature);

/**
 * @brief Read pH with temperature compensation
 * @param dev I2C device descriptor
 * @param temperature This value is required for temperature compensation
 * @param ph pointer to ph variable
 * @return ESP_OK to indicate success
 */
esp_err_t read_ph_with_temperature(ph_sensor_t *dev, float temperature, float *ph);

/**
 * @brief Read pH without temperature compensation
 * @param dev I2C device descriptor
 * @param ph pointer to ph variable
 * @return ESP_OK to indicate success
 */
esp_err_t read_ph(ph_sensor_t *dev, float *ph);

#ifdef __cplusplus
}
#endif

#endif /* PH_SENSOR_H */


