/*
 * ec_sensor.h
 *
 *  Created on: May 1, 2020
 *      Author: ajayv
 */

#ifndef EC_SENSOR_H
#define EC_SENSOR_H

#include <esp_err.h>
#include "i2cdev.h"
#define EC_ADDR_BASE 0x64

#ifdef __cplusplus
extern "C" {
#endif

typedef i2c_dev_t ec_sensor_t;

/**
 * @brief Setup EC I2C communication
 * @param dev I2C device descriptor
 * @param port I2C port
 * @param addr I2C address
 * @param sda_gpio SDA GPIO
 * @param scl_gpio SCL GPIO
 * @return any error message
 */
esp_err_t ec_init(ec_sensor_t *dev, i2c_port_t port, uint8_t addr, int8_t sda_gpio, int8_t scl_gpio);

/**
 * @brief Calibrate EC sensor
 * @param dev I2C device descriptor
 * @param temperature This value is required for temperature compensation
 * @return any error message
 */
esp_err_t calibrate_ec(ec_sensor_t *dev);

/**
 * @brief Calibrate EC sensor with dry mode
 * @param dev I2C device descriptor
 * @return any error message
 */
esp_err_t calibrate_ec_dry(ec_sensor_t *dev);

/**
 * @brief Read EC with temperature compensation
 * @param dev I2C device descriptor
 * @param temperature This value is required for temperature compensation
 * @param EC pointer to EC variable
 * @return any error message
 */
esp_err_t read_ec_with_temperature(ec_sensor_t *dev, float temperature, float *ec);

/**
 * @brief Read EC without temperature compensation
 * @param dev I2C device descriptor
 * @param EC pointer to EC variable
 * @return any error message
 */
esp_err_t read_ec(ec_sensor_t *dev, float *ec);

#ifdef __cplusplus
}
#endif

#endif /* EC_SENSOR_H_ */


