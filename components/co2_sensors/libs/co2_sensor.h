/*
 * co2_sensor.h
 *
 *  Created on: May 30, 2021
 *      Author: Karthick Siva. 
 */

#ifndef CO2_SENSOR_H
#define CO2_SENSOR_H

#include <esp_err.h>
#include "i2cdev.h"

#define CO2_ADDR_BASE 0x69

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
 * @brief Read CO2 
 * @param dev I2C device descriptor
 * @param co2 pointer to co2 variable
 * @return ESP_OK to indicate success
 */
esp_err_t read_co2(co2_sensor_t *dev, uint_32 co2);


#ifdef __cplusplus
}
#endif

#endif /* PH_SENSOR_H */


