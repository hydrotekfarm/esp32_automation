#include "rf_transmitter.h"
#include "ports.h"

#include <Freertos/freertos.h>
#include <freertos/task.h>
#include <string.h>
#include <esp_log.h>

void init_rf_protocol() {
	// Setup Transmission Protocol
	gpio_set_direction(RF_TRANSMITTER_GPIO, GPIO_MODE_OUTPUT);

	struct binary_bits low_bit = {3, 1};
	struct binary_bits sync_bit = {31, 1};
	struct binary_bits high_bit = {1, 3};
	configure_protocol(172, 10, RF_TRANSMITTER_GPIO, low_bit, high_bit, sync_bit);
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
	generate_rf_address(water_cooler_address, address_index + RF_BASE_ADDRESS_WATER_COOLER);
	generate_rf_address(water_heater_address, address_index + RF_BASE_ADDRESS_WATER_HEATER);
	generate_rf_address(water_in_address, address_index + RF_BASE_ADDRESS_WATER_IN);
	generate_rf_address(water_out_address, address_index + RF_BASE_ADDRESS_WATER_OUT);
	for(int i = 0; i < grow_light_arr_current_length; i++) {
		generate_rf_address(grow_light_address[i], address_index + RF_BASE_ADDRESS_GROW_LIGHTS + i);
	}
}

void rf_transmitter(void *parameter) {
	init_rf_protocol();
	init_rf_addresses();

	struct rf_message message;
	rf_transmitter_queue = xQueueCreate(1, sizeof(message));

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

