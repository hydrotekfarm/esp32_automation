#include "port_manager.h"

#include <esp_log.h>
#include <esp_err.h>

void port_setup() {								// ADC Port Setup Method
	// ADC 1 setup
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12,
			DEFAULT_VREF, adc_chars);

	// ADC Channel Setup
	adc1_config_channel_atten(ADC_CHANNEL_0, ADC_ATTEN_DB_11);
	adc1_config_channel_atten(ADC_CHANNEL_3, ADC_ATTEN_DB_11);

	gpio_set_direction(32, GPIO_MODE_OUTPUT);
	esp_err_t error = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF); 	// Check if built in ADC calibration is included in board
	if (error != ESP_OK) {
		ESP_LOGE(PORT_TAG, "EFUSE_VREF not supported, use a different ESP 32 board");
	}
}

void gpio_setup() {  // Setup GPIO ports that are controlled through high/low mechanism
	gpio_set_direction(PH_UP_PUMP_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_direction(PH_DOWN_PUMP_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_direction(EC_NUTRIENT_1_PUMP_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_direction(EC_NUTRIENT_2_PUMP_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_direction(EC_NUTRIENT_3_PUMP_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_direction(EC_NUTRIENT_4_PUMP_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_direction(EC_NUTRIENT_5_PUMP_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_direction(EC_NUTRIENT_6_PUMP_GPIO, GPIO_MODE_OUTPUT);
}
