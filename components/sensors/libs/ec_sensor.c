/*
 * ec_sensor.c
 *
 *  Created on: May 1, 2020
 *      Author: ajayv, Karthick Siva. 
 */

#include "ec_sensor.h"
#include <esp_log.h>
#include <esp_err.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
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

esp_err_t activate_ec(ec_sensor_t *dev) {
	char data = 0x01; 
	char reg = 0x06; 
    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, &reg, sizeof(reg), &data, sizeof(data)));
    I2C_DEV_GIVE_MUTEX(dev);
	return ESP_OK; 
}

esp_err_t hibernate_ec(ec_sensor_t *dev) {
	char data = 0x00; 
	char reg = 0x06; 
    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, &reg, sizeof(reg), &data, sizeof(data)));
    I2C_DEV_GIVE_MUTEX(dev);
	return ESP_OK; 
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
	char calib_point = 3; 
	unsigned char msb = 0x00; 
	unsigned char high_byte = 0x00; 
	unsigned char low_byte = 0x00; 
	unsigned char lsb = 0x00; 
	if(ec >= 5 && ec < 20) {
		low_byte = 0x05; 
		lsb = 0x08;
		ESP_LOGI(TAG, "12.88 millisiemens solution identified");
	} else {
		ESP_LOGE(TAG, "calibration solution not identified, ec is lower than 7 millisiemens or greater than 90 millisiemens");
		return ESP_FAIL;
	}

	// Send Calibration Command to EZO Sensor
	char msb_reg = 0x0A;
	char high_reg = 0x0B; 
	char low_reg = 0x0C; 
	char lsb_reg = 0x0D; 
    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, &msb_reg, sizeof(msb_reg), &msb, sizeof(msb)));
	I2C_DEV_CHECK(dev, i2c_dev_write(dev, &high_reg, sizeof(high_reg), &high_byte, sizeof(high_byte)));
	I2C_DEV_CHECK(dev, i2c_dev_write(dev, &low_reg, sizeof(low_reg), &low_byte, sizeof(low_byte)));
	I2C_DEV_CHECK(dev, i2c_dev_write(dev, &lsb_reg, sizeof(lsb_reg), &lsb, sizeof(lsb)));
    I2C_DEV_GIVE_MUTEX(dev);
    vTaskDelay(pdMS_TO_TICKS(1000));	// Processing Delay

	//Calibration request Register//
	char calib_req_reg = 0x0E; 
	I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, &calib_req_reg, sizeof(calib_req_reg), &calib_point, sizeof(calib_point)));
    I2C_DEV_GIVE_MUTEX(dev);
    vTaskDelay(pdMS_TO_TICKS(1000));	// Processing Delay

	//Calibration Confirmation register//
	char calib_confirm_reg = 0x0F; 
	char output = -1; 
	I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_read(dev, &calib_confirm_reg, sizeof(calib_confirm_reg), &output, sizeof(output)));
    I2C_DEV_GIVE_MUTEX(dev);

	switch (calib_point) {
		//if 12.88 solution //
		case 3: 
			if (output == 2 || output == 3) {
				ESP_LOGI(TAG, "Single Point 12.88 millisiemen calibration set");
			} else {
				ESP_LOGE(TAG, "Single Point 12.88 millisiemen calibration uanble to be set");
				return ESP_FAIL; 
			}
			break;
		default: 
			ESP_LOGE(TAG, "Unable to confirm calibration.");
			return ESP_FAIL; 
	}
	return ESP_OK;
}

esp_err_t calibrate_ec_dry(ec_sensor_t *dev) {
	float ec = 0;
	for (int i = 0; i < DRY_CALIBRATION_READING_COUNT; i++) {
		read_ec(dev, &ec);
	}
	// Create Calibration Command
	char calib_point = 2; 
	char calib_req_reg = 0x0E; 
	I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, &calib_req_reg, sizeof(calib_req_reg), &calib_point, sizeof(calib_point)));
    I2C_DEV_GIVE_MUTEX(dev);
    vTaskDelay(pdMS_TO_TICKS(1000));	// Processing Delay

	//Calibration Confirmation register//
	char calib_confirm_reg = 0x0F; 
	char output = -1; 
	I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_read(dev, &calib_confirm_reg, sizeof(calib_confirm_reg), &output, sizeof(output)));
    I2C_DEV_GIVE_MUTEX(dev);

	if (output == 1 || output == 3) {
		ESP_LOGI(TAG, "Dry calibration set");
	} else {
		ESP_LOGE(TAG, "Dry calibration uanble to be set");
		return ESP_FAIL; 
	}
	return ESP_OK;
}

esp_err_t clear_calibration_ec(ec_sensor_t *dev) {
	//Calibration request Register//
	char calib_req_reg = 0x0E; 
	char calib_point = 1; 
	I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, &calib_req_reg, sizeof(calib_req_reg), &calib_point, sizeof(calib_point)));
    I2C_DEV_GIVE_MUTEX(dev);
    vTaskDelay(pdMS_TO_TICKS(1000));	// Processing Delay

	//Calibration Confirmation register//
	char calib_confirm_reg = 0x0F; 
	char output = -1; 
	I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_read(dev, &calib_confirm_reg, sizeof(calib_confirm_reg), &output, sizeof(output)));
    I2C_DEV_GIVE_MUTEX(dev);

	if (output == 0) {
		ESP_LOGI(TAG, "Calibration data cleared");
	} else {
		ESP_LOGE(TAG, "Calibration data not cleared");
		return ESP_FAIL; 
	}
	return ESP_OK; 
}

esp_err_t read_ec_with_temperature(ec_sensor_t *dev, float temperature, float *ec) {
	// Create Read with temperature command
	unsigned int temp_compensation = temperature * 100; 
	unsigned char msb = 0x00; 
	unsigned char high_byte = 0x00; 
	unsigned char low_byte = (temp_compensation>>8) & 0xFF; 
	unsigned char lsb = temp_compensation & 0xFF; 
	char msb_reg = 0x10;
	char high_reg = 0x11; 
	char low_reg = 0x12; 
	char lsb_reg = 0x13; 
	I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, &msb_reg, sizeof(msb_reg), &msb, sizeof(msb)));
	I2C_DEV_CHECK(dev, i2c_dev_write(dev, &high_reg, sizeof(high_reg), &high_byte, sizeof(high_byte)));
	I2C_DEV_CHECK(dev, i2c_dev_write(dev, &low_reg, sizeof(low_reg), &low_byte, sizeof(low_byte)));
	I2C_DEV_CHECK(dev, i2c_dev_write(dev, &lsb_reg, sizeof(lsb_reg), &lsb, sizeof(lsb)));
    I2C_DEV_GIVE_MUTEX(dev);
    vTaskDelay(pdMS_TO_TICKS(1000));	// Processing Delay

	//Temperature Compensation Confirmation// 
	int count = 0; 
	unsigned char bytes [4]; 
	float check_temp = 0.0f; 
	while (check_temp != temperature) {
		if (count == 3) {
			ESP_LOGE(TAG, "Unable to set temperature compensation point.");
			return ESP_FAIL; 
		} 
		msb_reg = 0x14; 
		high_reg = 0x15;
		low_reg = 0x16;
		lsb_reg = 0x17; 
		I2C_DEV_TAKE_MUTEX(dev);
    	I2C_DEV_CHECK(dev, i2c_dev_read(dev, &msb_reg, sizeof(msb_reg), &msb, sizeof(msb)));
		bytes[0] = msb; 
		I2C_DEV_CHECK(dev, i2c_dev_read(dev, &high_reg, sizeof(high_reg), &high_byte, sizeof(high_byte)));
		bytes[1] = high_byte; 
		I2C_DEV_CHECK(dev, i2c_dev_read(dev, &low_reg, sizeof(low_reg), &low_byte, sizeof(low_byte)));
		bytes[2] = low_byte;  
		I2C_DEV_CHECK(dev, i2c_dev_read(dev, &lsb_reg, sizeof(lsb_reg), &lsb, sizeof(lsb)));
		bytes[3] = lsb; 
    	I2C_DEV_GIVE_MUTEX(dev);
		int check = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);
		check_temp = ((float) check) / 100; 
		count++;
		vTaskDelay(pdMS_TO_TICKS(1000));
	}


	//Commands to recieve ph data//
	char new_reading_reg = 0x07; 
	int new_reading = 0; 
	count = 0; 
	while (new_reading == 0) {
		if (count == 3) {
			ESP_LOGE(TAG, "Unable to get new ec reading.");
			return ESP_FAIL; 
		} 
		I2C_DEV_TAKE_MUTEX(dev);
    	I2C_DEV_CHECK(dev, i2c_dev_read(dev, &new_reading_reg, sizeof(new_reading_reg), &new_reading, sizeof(new_reading)));
		I2C_DEV_GIVE_MUTEX(dev);
		vTaskDelay(pdMS_TO_TICKS(1000));
		if (new_reading == 1) {
			//reset back to 0//
			char reset = 0; 
			I2C_DEV_TAKE_MUTEX(dev);
    		I2C_DEV_CHECK(dev, i2c_dev_write(dev, &new_reading_reg, sizeof(new_reading_reg), &reset, sizeof(reset)));
			I2C_DEV_GIVE_MUTEX(dev);
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
		count++; 
	}

	//get new ec data// 
	msb_reg = 0x18; 
	high_reg = 0x19;
	low_reg = 0x1A;
	lsb_reg = 0x1B; 
	I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_read(dev, &msb_reg, sizeof(msb_reg), &msb, sizeof(msb)));
	bytes[0] = msb; 
	I2C_DEV_CHECK(dev, i2c_dev_read(dev, &high_reg, sizeof(high_reg), &high_byte, sizeof(high_byte)));
	bytes[1] = high_byte; 
	I2C_DEV_CHECK(dev, i2c_dev_read(dev, &low_reg, sizeof(low_reg), &low_byte, sizeof(low_byte)));
	bytes[2] = low_byte;  
	I2C_DEV_CHECK(dev, i2c_dev_read(dev, &lsb_reg, sizeof(lsb_reg), &lsb, sizeof(lsb)));
	bytes[3] = lsb; 
    I2C_DEV_GIVE_MUTEX(dev);
	int val = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);
	*ec = ((float) val) / 100; 

    return ESP_OK;
}

esp_err_t read_ec(ec_sensor_t *dev, float *ec) {
	//Commands to recieve ph data//
	char new_reading_reg = 0x07; 
	int new_reading = 0; 
	int count = 0; 
	while (new_reading == 0) {
		if (count == 3) {
			ESP_LOGE(TAG, "Unable to get new ec reading.");
			return ESP_FAIL; 
		} 
		I2C_DEV_TAKE_MUTEX(dev);
    	I2C_DEV_CHECK(dev, i2c_dev_read(dev, &new_reading_reg, sizeof(new_reading_reg), &new_reading, sizeof(new_reading)));
		I2C_DEV_GIVE_MUTEX(dev);
		vTaskDelay(pdMS_TO_TICKS(1000));
		if (new_reading == 1) {
			//reset back to 0//
			char reset = 0; 
			I2C_DEV_TAKE_MUTEX(dev);
    		I2C_DEV_CHECK(dev, i2c_dev_write(dev, &new_reading_reg, sizeof(new_reading_reg), &reset, sizeof(reset)));
			I2C_DEV_GIVE_MUTEX(dev);
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
		count++; 
	}

	//get new ec data// 
	unsigned char bytes [4];
	char msb_reg = 0x18; 
	char high_reg = 0x19;
	char low_reg = 0x1A;
	char lsb_reg = 0x1B; 
	unsigned char msb = 0x00; 
	unsigned char high_byte = 0x00; 
	unsigned char low_byte = 0x00; 
	unsigned char lsb = 0x00; 
	I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_read(dev, &msb_reg, sizeof(msb_reg), &msb, sizeof(msb)));
	bytes[0] = msb; 
	I2C_DEV_CHECK(dev, i2c_dev_read(dev, &high_reg, sizeof(high_reg), &high_byte, sizeof(high_byte)));
	bytes[1] = high_byte; 
	I2C_DEV_CHECK(dev, i2c_dev_read(dev, &low_reg, sizeof(low_reg), &low_byte, sizeof(low_byte)));
	bytes[2] = low_byte;  
	I2C_DEV_CHECK(dev, i2c_dev_read(dev, &lsb_reg, sizeof(lsb_reg), &lsb, sizeof(lsb)));
	bytes[3] = lsb; 
    I2C_DEV_GIVE_MUTEX(dev);
	int val = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);
	*ec = ((float) val) / 100; 

    return ESP_OK;
}







