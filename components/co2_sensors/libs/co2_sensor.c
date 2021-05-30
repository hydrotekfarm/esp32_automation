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
#define STABILIZATION_ACCURACY 0.002
#define STABILIZATION_COUNT_MAX 10

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
/*
esp_err_t calibrate_ph(ph_sensor_t *dev, float temperature){
	uint8_t count = 0;

	float ph = 0;
	float ph_min = 0;
	float ph_max = 0;

	// Keep restarting until 10 consecutive ph values are within stabilization accuracy range
	while(count < STABILIZATION_COUNT_MAX){
		esp_err_t err = read_ph_with_temperature(dev, temperature, &ph);	// read ph with temperature
		ESP_LOGI(TAG, "ph: %f", ph);
		if (err == ESP_OK) {	// Proceed if ph sensor responds with success code
			if(count == 0) {	// If first reading, then calculate stabilization range
				ph_min = ph * (1 - STABILIZATION_ACCURACY);
				ph_max = ph * (1 + STABILIZATION_ACCURACY);
				ESP_LOGI(TAG, "min ph: %f, max ph: %f", ph_min, ph_max);
				count++;
			} else {
				if(ph >= ph_min && ph <= ph_max){	// increment count if ph is within range
					count++;
				} else {
					count = 0;	// reset count to zero if ph is not within range
				}
			}
		} else {
			ESP_LOGI(TAG, "response code: %d", err);
		}
	}

	// Identify and create calibration command
	char cmd[15];
	memset(&cmd, 0, sizeof(cmd));
	if(ph >= 2.5 && ph < 5.5) {
		ESP_LOGI(TAG, "4.0 solution identified");
		snprintf(cmd, sizeof(cmd), "cal,low,4");
	} else if (ph >= 5.5 && ph <= 8.5) {
		ESP_LOGI(TAG, "7.0 solution identified");
		snprintf(cmd, sizeof(cmd), "cal,mid,7");
	} else if (ph > 8.5 && ph <= 11.5) {
		ESP_LOGI(TAG, "10.0 solution identified");
		snprintf(cmd, sizeof(cmd), "cal,high,10");
	} else {
		ESP_LOGE(TAG, "calibration solution not identified, ph is lower than 2.5 or greater than 11.5");
		return ESP_FAIL;
	}

	// Send Calibration Command to EZO Sensor
    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, &cmd, sizeof(cmd)));
    I2C_DEV_GIVE_MUTEX(dev);
    vTaskDelay(pdMS_TO_TICKS(1000));	// Processing Delay
	return ESP_OK;
}
*/

esp_err_t read_co2(co2_sensor_t *dev, uint_32 *co2) {
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
	vTaskDelay(pdMS_TO_TICKS(10000));

	// read ph from atlas sensor and store it in buffer
	I2C_DEV_TAKE_MUTEX(dev);
	I2C_DEV_CHECK(dev, i2c_read_ezo_sensor(dev, &response_code, buffer, 31));
	I2C_DEV_GIVE_MUTEX(dev);

	// convert buffer into a float and store it in ph variable
	*ph = atoi(buffer);

    // return any error
    if(response_code != 1) {
    	return response_code;
    }
	return ESP_OK;
}

