/*
 * ec_sensor.c
 *
 *  Created on: May 1, 2020
 *      Author: ajayv
 */

#include "ec_sensor.h"
#include <esp_log.h>
#include <esp_err.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

#define I2C_FREQ_HZ 400000
#define STABILIZATION_ACCURACY 0.002
#define STABILIZATION_COUNT_MAX 10
#define DRY_CALIBRATION_READING_COUNT 20

static const char *TAG = "Atlas EC Sensor";

esp_err_t ec_init(ec_sensor_t *dev, i2c_port_t port, uint8_t addr, int8_t sda_gpio, int8_t scl_gpio) {
	// Check Arguments
    CHECK_ARG(dev);
    if (addr < EC_ADDR_BASE || addr > EC_ADDR_BASE + 7)
    {
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

esp_err_t calibrate_ec(ec_sensor_t *dev){
	uint8_t count = 0;

	float ec = 0;
	float ec_min = 0;
	float ec_max = 0;

	// Keep restarting until 10 consecutive ph values are within stabilization accuracy range
	while(count < STABILIZATION_COUNT_MAX){
		esp_err_t err = read_ec(dev, &ec);	// read ec with temperature
		ESP_LOGI(TAG, "ec: %f", ec);
		if (err == ESP_OK) {	// Proceed if ec sensor responds with success code
			if(count == 0) {	// If first reading, then calculate stabilization range
				ec_min = ec * (1 - STABILIZATION_ACCURACY);
				ec_max = ec * (1 + STABILIZATION_ACCURACY);
				ESP_LOGI(TAG, "min ec: %f, max ec: %f", ec_min, ec_max);
				count++;
			} else {
				if(ec >= ec_min && ec <= ec_max){	// increment count if ec is within range
					count++;
				} else {
					count = 0;	// reset count to zero if ec is not within range
				}
			}
		} else {
			ESP_LOGI(TAG, "response code: %d", err);
		}
	}

	// Identify and create calibration command
	char cmd[20];
	memset(&cmd, 0, sizeof(cmd));
	if(ec >= 5 && ec < 20) {
		ESP_LOGI(TAG, "12.88 millisiemens solution identified");
		snprintf(cmd, sizeof(cmd), "cal,12880");
	} else {
		ESP_LOGE(TAG, "calibration solution not identified, ec is lower than 7 millisiemens or greater than 90 millisiemens");
		return ESP_FAIL;
	}
    vTaskDelay(pdMS_TO_TICKS(1000));	// Processing Delay
	// Send Calibration Command to EZO Sensor
    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, &cmd, sizeof(cmd)));
    I2C_DEV_GIVE_MUTEX(dev);
    vTaskDelay(pdMS_TO_TICKS(1000));	// Processing Delay
	return ESP_OK;
}

esp_err_t calibrate_ec_dry(ec_sensor_t *dev) {
	float ec = 0;
	for (int i = 0; i < DRY_CALIBRATION_READING_COUNT; i++) {
		read_ec(dev, &ec);
	}
	// Create Calibration Command
	char cmd[20];
	memset(&cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "cal,dry");

	// Send Calibration Command to EZO Sensor
    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, &cmd, sizeof(cmd)));
    I2C_DEV_GIVE_MUTEX(dev);
    vTaskDelay(pdMS_TO_TICKS(1000));	// Processing Delay

	return ESP_OK;
}

esp_err_t read_ec_with_temperature(ec_sensor_t *dev, float temperature, float *ec) {
	uint8_t response_code = 0;

	// Create Read with temperature command
	char cmd[10];
	memset(&cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "RT,%.1f", temperature);

	// Create buffer to read data
	char buffer[32];
	memset(&buffer, 0, sizeof(buffer));

	// write read with temperature command
    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, &cmd, sizeof(cmd)));
    I2C_DEV_GIVE_MUTEX(dev);

    // processing delay for atlas sensor
    vTaskDelay(pdMS_TO_TICKS(900));

    // read ec from atlas sensor and store it in buffer
    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_read_ezo_sensor(dev, &response_code, buffer, 31));
    I2C_DEV_GIVE_MUTEX(dev);

    // convert buffer into a float and convert to milliseimens and store it in ec variable
    *ec = atof(buffer)/1000;
    // return any error
    if(response_code != 1) {
    	return response_code;
    }
    return ESP_OK;
}

esp_err_t read_ec(ec_sensor_t *dev, float *ec) {
	uint8_t response_code = 0;
	char cmd = 'R';

	// Create buffer to read data
	char buffer[32];
	memset(&buffer, 0, sizeof(buffer));

	// write read ec command
	I2C_DEV_TAKE_MUTEX(dev);
	I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, &cmd, sizeof(cmd)));
	I2C_DEV_GIVE_MUTEX(dev);

	// processing delay for atlas sensor
	vTaskDelay(pdMS_TO_TICKS(900));

	// read ec from atlas sensor and store it in buffer
	I2C_DEV_TAKE_MUTEX(dev);
	I2C_DEV_CHECK(dev, i2c_read_ezo_sensor(dev, &response_code, buffer, 31));
	I2C_DEV_GIVE_MUTEX(dev);

	// convert buffer into a float and convert it to milliseimens and store it in ec variable
	*ec = atof(buffer)/1000;

    // return any error
    if(response_code != 1) {
    	return response_code;
    }
	return ESP_OK;
}







