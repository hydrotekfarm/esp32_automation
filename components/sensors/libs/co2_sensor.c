/*
 * co2_sensor.c
 *
 *  Created on: May 30, 2020
 *      Author: Karthick Siva
 */

#include "co2_sensor.h"
#include <esp_log.h>
#include <esp_err.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>


#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

#define I2C_FREQ_HZ 400000

static const char *TAG = "Atlas CO2 Sensor";


esp_err_t co2_init(co2_sensor_t *dev, i2c_port_t port, uint8_t addr, int8_t sda_gpio, int8_t scl_gpio) {
	// Check Arguments
    CHECK_ARG(dev);

    if (addr < CO2_ADDR_BASE || addr > CO2_ADDR_BASE + 7) {
        ESP_LOGE(TAG, "Invalid device address: 0x%02x", addr);
        return ESP_ERR_INVALID_ARG;
    }

    // Setup I2C settings
    dev->port = port;
    dev->addr = addr;
    dev->cfg.sda_io_num = sda_gpio;
    dev->cfg.scl_io_num = scl_gpio;
    dev->cfg.master.clk_speed = I2C_FREQ_HZ;

    return i2c_dev_create_mutex(dev);
}

esp_err_t read_co2(co2_sensor_t *dev, float *co2) {
	uint8_t response_code = 0;
	char cmd = 'R';
	// Create buffer to read data
	char buffer[32];
	memset(&buffer, 0, sizeof(buffer));
	// write read co2 command
	I2C_DEV_TAKE_MUTEX(dev);
	I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, &cmd, sizeof(cmd)));
	I2C_DEV_GIVE_MUTEX(dev);

	// processing delay for atlas sensor
	vTaskDelay(pdMS_TO_TICKS(900));

	// read co2 from atlas sensor and store it in buffer
	I2C_DEV_TAKE_MUTEX(dev);
	I2C_DEV_CHECK(dev, i2c_read_ezo_sensor(dev, &response_code, buffer, 31));
	I2C_DEV_GIVE_MUTEX(dev);

	// convert buffer  and store it in co2 variable
	*co2 = atoi(buffer);

    // return any error
    if(response_code != 1) {
    	return response_code;
    }
	return ESP_OK;
}

