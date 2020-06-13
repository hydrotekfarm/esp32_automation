/*
 * rf_transmission.h
 *
 *  Created on: Jun 12, 2020
 *      Author: ajayv
 */

#ifndef COMPONENTS_RF_TRANSMISSION_RF_TRANSMISSION_H_
#define COMPONENTS_RF_TRANSMISSION_RF_TRANSMISSION_H_

#include <freertos/FreeRTOS.h>

struct binary_bits{
	uint32_t low_pulse_amount;
	uint32_t high_pulse_amount;
};

struct protocol {
	uint32_t pulse_width;
	struct binary_bits low_bit;
	struct binary_bits high_bit;
	struct binary_bits sync_bit;
	uint32_t repeat_transmission;
	uint16_t transmit_pin;
} power_outlet_protocol;

void configure_protocol(int32_t pulse_width, int32_t repeat_transmission, int16_t transmit_pin, struct binary_bits low_bit, struct binary_bits high_bit, struct binary_bits sync_bit);

void send_message(const char* binary_code);

#endif /* COMPONENTS_RF_TRANSMISSION_RF_TRANSMISSION_H_ */
