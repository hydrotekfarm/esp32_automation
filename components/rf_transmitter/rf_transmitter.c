#include "rf_transmitter.h"

#include <Freertos/freertos.h>
#include <freertos/task.h>

void send_rf_transmission(){
	// Setup Transmission Protocol
	struct binary_bits low_bit = {3, 1};
	struct binary_bits sync_bit = {31, 1};
	struct binary_bits high_bit = {1, 3};
	configure_protocol(172, 10, 32, low_bit, high_bit, sync_bit);

	// Start controlling outlets
	send_message("000101000101010100110011"); // Binary Code to turn on Power Outlet 1
	vTaskDelay(pdMS_TO_TICKS(5000));
	send_message("000101000101010100111100"); // Binary Code to turn off Power Outlet 1
	vTaskDelay(pdMS_TO_TICKS(5000));
}

void init_rf() {
	low_bit.low_pulse_amount = 3;
	low_bit.high_pulse_amount = 1;
	sync_bit.low_pulse_amount = 31;
	sync_bit.high_pulse_amount = 1;
	high_bit.low_pulse_amount = 1;
	high_bit.high_pulse_amount = 3;
}
