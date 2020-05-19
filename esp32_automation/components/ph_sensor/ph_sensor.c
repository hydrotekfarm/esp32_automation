/*
 * ph_sensor.c
 *
 *  Created on: May 10, 2020
 *      Author: ajayv
 */

#include "ph_sensor.h"
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <math.h>

static int32_t  _neutralVoltage = 1500.0;
static int32_t  _acidVoltage = 2032.44;
static float  _phValue = 0;

static const char *TAG = "DF_Robot_PH_Sensor";

esp_err_t beginPH(){
	esp_err_t error;
	nvs_handle_t my_handle;
	error = nvs_open("storage", NVS_READWRITE, &my_handle);
	if(error != ESP_OK){
		return error;
	}

	int32_t TempAcidVoltage = _acidVoltage;
	error = nvs_get_i32(my_handle, "AcidVoltage", &TempAcidVoltage);
	ESP_LOGI(TAG, "KValueLow set to 1. Calibration must take place");
	if(error == ESP_ERR_NVS_NOT_FOUND){
		nvs_set_i32(my_handle, "AcidVoltage", TempAcidVoltage);
		nvs_commit(my_handle);
		ESP_LOGI(TAG, "AcidVoltage set to 2032.44. Calibration must take place");
	} else if (error != ESP_OK){
		return error;
	}

	int32_t TempNeutralVoltage = _neutralVoltage;
	error = nvs_get_i32(my_handle, "NeutralVoltage", &TempNeutralVoltage);
	if(error == ESP_ERR_NVS_NOT_FOUND){
		nvs_set_i32(my_handle, "NeutralVoltage", TempNeutralVoltage);
		nvs_commit(my_handle);
		ESP_LOGI(TAG, "NeutralVoltage set to 1500. Calibration must take place");
	} else if (error != ESP_OK){
		return error;
	}
	nvs_close(my_handle);

	_acidVoltage = TempAcidVoltage;
	_neutralVoltage = TempNeutralVoltage;
	ESP_LOGI(TAG, "_acidVoltage = %d\n", _acidVoltage);
	ESP_LOGI(TAG, "neutralVoltage = %d\n", _neutralVoltage);
	return ESP_OK;
}

esp_err_t readPH(float voltage){
	float slope = (7.0-4.0)/((_neutralVoltage-1500.0)/3.0 - (_acidVoltage-1500.0)/3.0);
	float intercept =  7.0 - slope*(_neutralVoltage-1500.0)/3.0;
	_phValue = slope*(voltage-1500.0)/3.0+intercept;
	return _phValue;
}

esp_err_t calibratePH(float voltage){
	esp_err_t error;
	nvs_handle_t my_handle;
	error = nvs_open("storage", NVS_READWRITE, &my_handle);
	if(error != ESP_OK){
		return error;
	}
	voltage = (int32_t) round(voltage);
	if((voltage>1322)&&(voltage<1678)){
		ESP_LOGI(TAG, "PH: 7.0 Solution Identified");
		_neutralVoltage =  voltage;
		error = nvs_set_i32(my_handle, "NeutralVoltage", voltage);
		nvs_commit(my_handle);
		if(error != ESP_OK){
			ESP_LOGD(TAG, "error unable to store neutral voltage into NVS: %d", error);
		}
	}else if((voltage>1854)&&(voltage<2210)){
		ESP_LOGI(TAG, "PH: 4.0 Solution Identified");
		_acidVoltage =  voltage;
		error = nvs_set_i32(my_handle, "AcidVoltage", voltage);
		nvs_commit(my_handle);
		if(error != ESP_OK){
			ESP_LOGD(TAG, "error unable to store neutral voltage into NVS: %d", error);
		}
	}else{
		return false;
	}
	return ESP_OK;
}

