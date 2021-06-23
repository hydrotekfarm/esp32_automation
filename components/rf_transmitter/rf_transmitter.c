#include "rf_transmitter.h"

#include <Freertos/freertos.h>
#include <freertos/task.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <cJSON.h>

#include "ports.h"
#include "mqtt_manager.h"

void init_rf_protocol() {
	// Setup Transmission Protocol
	struct binary_bits low_bit = {3, 1};
	struct binary_bits sync_bit = {31, 1};
	struct binary_bits high_bit = {1, 3};
	configure_protocol(172, 10, RF_TX_PIN, RF_RX_PIN, low_bit, high_bit, sync_bit);
}

void generate_rf_address(char *rf_address, long int current_num) {
    int index = RF_ADDRESS_LENGTH;
    rf_address[index--] = '\0';
    while(current_num != 0) {
        rf_address[index--] = current_num % 2 ? 49 : 48;
        current_num /= 2;
    }
    for( ; index >= 0; index--) rf_address[index] = 48;
}

void init_rf_addresses() {
	address_index = DEFAULT_ADDRESS_INDEX;	// get from NVS
	grow_light_arr_current_length = 3;		// get from NVS
	generate_rf_address(water_cooler_address, address_index + (int) WATER_COOLER);
	generate_rf_address(water_heater_address, address_index + (int) WATER_HEATER);
	generate_rf_address(water_in_address, address_index + (int) RESERVOIR_WATER_IN);
	generate_rf_address(water_out_address, address_index + (int) RESERVOIR_WATER_OUT);
	generate_rf_address(irrigation_address, address_index + (int) IRRIGATION);
	for(int i = 0; i < grow_light_arr_current_length; i++) {
		generate_rf_address(grow_lights_address[i], address_index + (int) GROW_LIGHTS + i);
	}
}

esp_err_t control_power_outlet(int power_outlet_id, bool state) {
	struct rf_message setup_rf_message;
	setup_rf_message.state = state;

	if(power_outlet_id == (int) WATER_COOLER) {
		setup_rf_message.rf_address_ptr = water_cooler_address;
	}

	else if(power_outlet_id == (int) WATER_HEATER) {
		setup_rf_message.rf_address_ptr = water_heater_address;
	}

	else if(power_outlet_id == (int) IRRIGATION) {
		setup_rf_message.rf_address_ptr = irrigation_address;
	}

	else if(power_outlet_id == (int) RESERVOIR_WATER_IN) {
		setup_rf_message.rf_address_ptr = water_in_address;
	}

	else if(power_outlet_id == (int) RESERVOIR_WATER_OUT) {
		setup_rf_message.rf_address_ptr = water_out_address;
	}

	else if(power_outlet_id >= (int) GROW_LIGHTS && power_outlet_id <= ((int) GROW_LIGHTS) + MAX_GROW_LIGHT_ZONES) {
		ESP_LOGI("Turning on grow lights", "Index number %d", power_outlet_id - (int) GROW_LIGHTS);
		setup_rf_message.rf_address_ptr = grow_lights_address[power_outlet_id - (int) GROW_LIGHTS];
	}
	else {
		return ESP_FAIL;
	}

	cJSON_SetNumberValue(get_rf_statuses()[power_outlet_id], state);
	publish_equipment_status();

	xQueueSend(rf_transmitter_queue, &setup_rf_message, pdMS_TO_TICKS(20000)); // TODO check if rf_message_address is not null (very important)
	ESP_LOGI(RF_TAG, "xqueue sent");
	return ESP_OK;

}

void rf_transmitter(void *parameter) {
	init_rf_protocol();
	init_rf_addresses();

	struct rf_message message;
	rf_transmitter_queue = xQueueCreate(20, sizeof(message));

	for(;;) {
		if(xQueueReceive(rf_transmitter_queue, &message, portMAX_DELAY)) {
			if(message.state == POWER_OUTLET_ON) {
				send_message(message.rf_address_ptr, on_binary_code);
			} else {
				send_message(message.rf_address_ptr, off_binary_code);
			}
		}
	}
}

void lights_on() {
	for(int i = 0; i < MAX_GROW_LIGHT_ZONES; ++i) {
		control_power_outlet(GROW_LIGHTS + i, true);
	}
}

void lights_off() {
	for(int i = 0; i < MAX_GROW_LIGHT_ZONES; ++i) {
		control_power_outlet(GROW_LIGHTS + i, false);
	}
}